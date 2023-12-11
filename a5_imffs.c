#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "a5_imffs.h"
#include "a5_multimap.h"

#define BLOCK_SIZE 256

struct KEY {
  void *file_name;
  u_int32_t file_size;
};

struct IMFFS {
  Multimap *index;
  u_int8_t *device;
  u_int8_t *free_blocks;
  uint32_t block_count;
};

int compare_keys_as_strings_case_insensitive(void *a, void *b) {
  struct KEY *left = (struct KEY *)a;
  struct KEY *right = (struct KEY *)b;
  return strcasecmp(left->file_name, right->file_name);
}

int compare_values_num_part(void *a, void *b) {
  Value *va = a, *vb = b;
  return va->num - vb->num;
}


static int imffs_find_empty_block(IMFFSPtr fs, int current_pos) {

  assert(fs != NULL);
  assert(current_pos >= 0);

  if(fs == NULL || current_pos < 0) {
    return IMFFS_INVALID;
  }

  for(u_int8_t i = current_pos; i <= fs->block_count; i++) {
    if(fs->free_blocks[i] == ' ') {
      return i;
    }
  }

  return -1;
}

// this function will create the filesystem with the given number of blocks;
// it will modify the fs parameter to point to the new file system or set it
// to NULL if something went wrong (fs is a pointer to a pointer)
IMFFSResult imffs_create(uint32_t block_count, IMFFSPtr *fs) {
  assert(block_count > 0);

  if(block_count <= 0) {
    return IMFFS_INVALID;
  }

  *fs = (IMFFSPtr)malloc(sizeof(struct IMFFS)); 
  
  if(*fs == NULL) {
    return IMFFS_ERROR;
  }

  (*fs)->index = mm_create(block_count, compare_keys_as_strings_case_insensitive, compare_values_num_part);
  (*fs)->device = (u_int8_t *)malloc(block_count * sizeof(u_int8_t) * BLOCK_SIZE);
  (*fs)->block_count = block_count;
  (*fs)->free_blocks = (u_int8_t *)malloc(block_count * sizeof(u_int8_t));

  if((*fs)->device == NULL || (*fs)->index == NULL || (*fs)->free_blocks == NULL) {
    mm_destroy(((*fs)->index));
    free((*fs)->free_blocks);
    free((*fs)->device);
    free(*fs);
    return IMFFS_FATAL;
  }

  for(u_int8_t i = 0; i < block_count; i++) {
    (*fs)->free_blocks[i] = ' ';
  }

  return IMFFS_OK;
}

// save diskfile imffsfile copy from your system to IMFFS
IMFFSResult imffs_save(IMFFSPtr fs, char *diskfile, char *imffsfile) {
  assert(fs != NULL);
  assert(diskfile != NULL);
  assert(imffsfile != NULL);

  if(fs == NULL || diskfile == NULL || imffsfile == NULL) {
    return IMFFS_INVALID;
  }
  
  char buffer[BLOCK_SIZE];
  FILE *source_file = fopen(diskfile, "rb");
  FILE *destination_file = fopen(imffsfile, "wb");

  if(source_file == NULL || destination_file == NULL) {
    return IMFFS_ERROR;
  }

  fseek(source_file, 0, SEEK_END);
  long size = ftell(source_file);
  fseek(source_file, 0, SEEK_SET);


  struct KEY *key = (struct KEY *)malloc(sizeof(struct KEY));
  key->file_name = malloc(strlen(imffsfile) + 1);
  strcpy(key->file_name, imffsfile);
  key->file_size = size;

  int startPos = 0;
  int empty_spot = imffs_find_empty_block(fs, startPos);
 
  int first_block_pos = empty_spot;
  size_t read, write;
  int num_block = 0;

  if(empty_spot >= 0) {
    //* okay found space;
    read = fread(buffer, sizeof(u_int8_t), BLOCK_SIZE, source_file);
    while(read > 0) { 
      //* if more data need to be read
      write = fwrite(buffer, sizeof(char), read, destination_file); //* write the data read so far
      fs->free_blocks[empty_spot] = 'X'; //* Mark that spot as used

      memcpy(&fs->device[BLOCK_SIZE * empty_spot], buffer, read);

      if(write != read) {
        fclose(source_file);
        fclose(destination_file);
        free(key);
        return IMFFS_ERROR;
      }
      num_block++;
      // //* check if the next index is empty
      if(fs->free_blocks[empty_spot + 1] == ' ' && (empty_spot + 1) < fs->block_count) {
        empty_spot++;      
      } else {
        //* insert the value until now
        u_int8_t *start_first_block = &fs->device[first_block_pos];
        mm_insert_value(fs->index, key, num_block, start_first_block);
        num_block = 0;
        empty_spot = imffs_find_empty_block(fs, ++startPos);
        first_block_pos = empty_spot;
      }
      
      read = fread(buffer, sizeof(u_int8_t), BLOCK_SIZE, source_file);
    }

    u_int8_t *start_first_block = &fs->device[first_block_pos];
    mm_insert_value(fs->index, key, num_block, start_first_block);
  } else {
    return IMFFS_ERROR;
  }

  fclose(source_file);
  fclose(destination_file);

  return IMFFS_OK;
}


// load imffsfile diskfile copy from IMFFS to your system
IMFFSResult imffs_load(IMFFSPtr fs, char *imffsfile, char *diskfile) {
  assert(fs != NULL);
  assert(diskfile != NULL);
  assert(imffsfile != NULL);

  if(fs == NULL || diskfile == NULL || imffsfile == NULL) {
    return IMFFS_INVALID;
  }

  FILE *destination_file = fopen(diskfile, "wb");

  if(destination_file == NULL) {
    return IMFFS_ERROR;
  }

  int item_found = 0;
  u_int32_t file_size = 0;

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {
      struct KEY *single_key = (struct KEY *)key;
      if(strcmp((char *)single_key->file_name, imffsfile) == 0) {
        item_found = 1;
        file_size = single_key->file_size;
        break;
      }
    } while (mm_get_next_key(fs->index, &key) > 0);
  }

  if(item_found == 0) {
    fclose(destination_file);
    return IMFFS_ERROR;
  }

  int total_values = mm_count_values(fs->index, key);
  Value data[total_values];
  int response = mm_get_values(fs->index, key, data, total_values);

  if(response == -1) {
    fclose(destination_file);
    return IMFFS_ERROR;
  }

  size_t read_size = file_size;

  size_t size_written = 0;
  
  for(int i = 0; i < total_values; i++) {
    int num_block = data[i].num;
    u_int8_t *start_first_block = data[i].data;
    size_t first_block_pos = (start_first_block - fs->device) / sizeof(u_int8_t);
    
    for(int j = 0; j < num_block; j++) {
      if(file_size >= BLOCK_SIZE) {
        size_written += fwrite(&fs->device[BLOCK_SIZE * (first_block_pos + j)], sizeof(char), BLOCK_SIZE, destination_file);
        file_size -= BLOCK_SIZE;
      } else {
        size_written += fwrite(&fs->device[BLOCK_SIZE * (first_block_pos + j)], sizeof(char), file_size, destination_file);
      }
    }
  }

  if(size_written !=read_size || size_written < 0) {
    return IMFFS_ERROR;
  }

  fclose(destination_file);
  return IMFFS_OK;
}

// delete imffsfile remove the IMFFS file from the system, allowing the blocks to be used for other files
IMFFSResult imffs_delete(IMFFSPtr fs, char *imffsfile) {
  assert(fs != NULL);
  assert(imffsfile != NULL);

  if(fs == NULL || imffsfile == NULL) {
    return IMFFS_INVALID;
  }

  int item_found = 0;
  u_int32_t file_size = 0;

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {
      struct KEY *single_key = (struct KEY *)key;
      if(strcmp((char *)single_key->file_name, imffsfile) == 0) {
        item_found = 1;
        file_size = single_key->file_size;
        break;
      }
    } while (mm_get_next_key(fs->index, &key) > 0);
  }

  if(file_size == 0 || item_found == 0) {
    return IMFFS_ERROR;
  }

  int total_values = mm_count_values(fs->index, key);
  Value data[total_values];
  int response = mm_get_values(fs->index, key, data, total_values);

  if(response == -1) {
    return IMFFS_ERROR;
  }

  for(int i = 0; i < total_values; i++) {
    int num_block = data[i].num;
    u_int8_t *start_first_block = data[i].data;
    size_t first_block_pos = (start_first_block - fs->device) / sizeof(u_int8_t);

    for(int j = 0; j < num_block; j++) {
      fs->free_blocks[first_block_pos + j] = ' ';
    }
  }

  int removed_keys = mm_remove_key(fs->index, key);

  if(removed_keys == -1) {
    return IMFFS_ERROR;
  }

  return IMFFS_OK;
}

// rename imffsold imffsnew rename the IMFFS file from imffsold to imffsnew, keeping all of the data intact
IMFFSResult imffs_rename(IMFFSPtr fs, char *imffsold, char *imffsnew) {

  assert(fs != NULL);
  assert(imffsold != NULL);
  assert(imffsnew != NULL);

  if(fs == NULL || imffsold == NULL || imffsnew == NULL) {
    return IMFFS_INVALID;
  }

  int item_found = 0;

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {
      struct KEY *single_key = (struct KEY *)key;
      if(strcmp((char *)single_key->file_name, imffsold) == 0) {
        item_found = 1;
        single_key->file_name = imffsnew;
        break;
      }
    } while (mm_get_next_key(fs->index, &key) > 0);
  }

  if(item_found == 0) {
    return IMFFS_ERROR;
  }

  return IMFFS_OK;
}

// dir will list all of the files and the number of bytes they occupy
IMFFSResult imffs_dir(IMFFSPtr fs) {
  assert(fs != NULL);

  if(fs == NULL) {
    return IMFFS_INVALID;
  }

  printf("___________DIR PRINTING DATA___________ \n");

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {
      struct KEY *single_key = (struct KEY *)key;
      printf("Filename: %s Space Occupied: %d (Bytes) \n", (char *)single_key->file_name, single_key->file_size);
    } while (mm_get_next_key(fs->index, &key) > 0);
  }

  printf("___________DIR PRINTING DATA END___________ \n");
  return IMFFS_OK;
}

// fulldir is like "dir" except it shows a the files and details about all of the chunks they are stored in (where, and how big)
IMFFSResult imffs_fulldir(IMFFSPtr fs) {

  assert(fs != NULL);

  if(fs == NULL) {
    return IMFFS_INVALID;
  }

  printf("___________FULL DIR PRINTING DATA___________ \n");

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {
      int values = mm_count_values(fs->index, key);
      Value data[values];
      int response = mm_get_values(fs->index, key, data, values);

      if(response == -1) {
        return IMFFS_ERROR;
      }

      struct KEY *single_key = (struct KEY *)key;
      for(int j = 0; j < values; j++) {
        u_int8_t *start_first_block = data[j].data;
        size_t first_block_pos = (start_first_block - fs->device) / sizeof(u_int8_t);
        printf("Filename: %s Space Occupied: %d (Bytes) Block: %d Chunck : %zu \n", (char *)single_key->file_name, single_key->file_size, data[j].num, first_block_pos);
      }
    } while (mm_get_next_key(fs->index, &key) > 0);
  }

  printf("___________FULL DIR PRINTING DATA END___________ \n");
  return IMFFS_OK;
}

// quit will quit the program: clean up the data structures
IMFFSResult imffs_destroy(IMFFSPtr fs) {
  assert(fs != NULL);

  if(fs == NULL) {
    return IMFFS_INVALID;
  }

  mm_destroy(fs->index);
  free(fs->device);
  free(fs->free_blocks);
  free(fs);

  return IMFFS_OK;
}

// defrag will defragment the filesystem: if you haven't implemented it, have it print "feature not implemented" and return IMFFS_NOT_IMPLEMENTED
IMFFSResult imffs_defrag(IMFFSPtr fs) {
  assert(fs != NULL);

  if(fs == NULL) {
    return IMFFS_INVALID;
  }

  void *key;
  if (mm_get_first_key(fs->index, &key) > 0) {
    do {

      int values = mm_count_values(fs->index, key);
      Value data[values];
      int response = mm_get_values(fs->index, key, data, values);

      if(response == -1) {
        return IMFFS_INVALID;
      }

      //! first block
      u_int8_t *start_first_block = data[0].data;
      size_t first_block_pos = (start_first_block - fs->device) / sizeof(u_int8_t);
      int empty_spot = imffs_find_empty_block(fs, 0);

      //! move back to empty space
      if(empty_spot < first_block_pos) {
        printf("space Need to move back");
        int num_block = data[0].num;

        for (int j = 0; j < num_block; j++) {
            
          memcpy(&fs->device[BLOCK_SIZE * empty_spot], &fs->device[BLOCK_SIZE * (first_block_pos + j)], BLOCK_SIZE);
          fs->free_blocks[empty_spot] = 'X';
          fs->free_blocks[first_block_pos + j] = ' ';
          empty_spot++;
        }

        mm_remove_key(fs->index, key);
        mm_insert_value(fs->index, key, num_block, &fs->device[(empty_spot - num_block)]);
      }

      if (values > 1) {
        // int swap_location = firstBlockPos + data[0].num;

        // for (int i = 1; i < values; i++) {
        //   int num_block = data[i].num;
        //   for (int j = 0; j < num_block; j++) {
        //     u_int8_t *start_other_block = data[i].data;
        //     size_t switching_with = (start_other_block - fs->device) / sizeof(u_int8_t);

        //     if(fs->free_blocks[swap_location] == ' ') {
        //       memcpy(&fs->device[BLOCK_SIZE * swap_location], &fs->device[BLOCK_SIZE * switching_with], BLOCK_SIZE);
        //       fs->free_blocks[swap_location] = 'X';
        //       fs->free_blocks[switching_with] = ' ';
        //     } else {
        //       u_int8_t block_data[BLOCK_SIZE];
        //       strcpy(block_data, &fs->device[BLOCK_SIZE * switching_with]);

        //       //! move it down
        //       for(int k = switching_with; k >= (swap_location - 1); k--) {
        //         memcpy(&fs->device[BLOCK_SIZE * k], &fs->device[BLOCK_SIZE * k - 1], BLOCK_SIZE);
        //       }

        //       memcpy(&fs->device[BLOCK_SIZE * swap_location], block_data, BLOCK_SIZE);
        //     }

        //     swap_location++;
        //   }
        //}
      }
      //! now look for partner
    } while (mm_get_next_key(fs->index, &key) > 0);
  }
  
  return IMFFS_OK;
}

