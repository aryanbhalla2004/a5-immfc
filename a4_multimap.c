#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "a4_multimap.h"


// Helper functions
static int find_key_pos(char *key, KeyAndValues *keys, int num_keys);
static int insert_key_alphabetically(KeyAndValues *keys, int keys_length, char *key);
static int insert_value_by_num(KeyAndValues *key, ValueNode *node);

// This gets rid of the warning about an unused function when assertions are off.
// It wouldn't be a problem if your function isn't "static" (which it doesn't have to be).
#ifndef NDEBUG
static int validate_multimap(Multimap *mm)
{
  assert(NULL != mm);
  assert(NULL != mm->keys);
  assert(mm->max_keys >= 0);
  assert(mm->num_keys >= 0 && mm->num_keys <= mm->max_keys);
  
  // Those were the easy/efficient ones, here is the tricky part to check the entire structure
  ValueNode *curr, *prev;
  int count;
  for (int i = 0; i < mm->num_keys; i++) {
    assert(strlen(mm->keys[i].key) < MAX_KEY_LENGTH);
    assert(mm->keys[i].num_values > 0); // can't have a key with no values
    assert(NULL != mm->keys[i].head);
    
    if (i > 0) {
      // ordering and duplication
      assert(strcasecmp(mm->keys[i-1].key, mm->keys[i].key) < 0);
    }
    
    count = 0;
    curr = mm->keys[i].head;
    prev = NULL;
    while (NULL != curr) {
      count++;
      assert(strlen(curr->value.str) < MAX_VALUE_LENGTH);
      if (prev != NULL) {
        // ordering
        assert(prev->value.num <= curr->value.num);
      }
      prev = curr;
      curr = curr->next;
    }
    assert(count == mm->keys[i].num_values);
  }
  
  return 1; // always return TRUE
}
#endif

Multimap *mm_create(int max_keys)
{
  assert(max_keys >= 0);
  
  Multimap *mm = NULL;
  
  if (max_keys >= 0) {
    mm = malloc(sizeof(Multimap));
    if (NULL != mm) {
      mm->keys = malloc(max_keys * sizeof(KeyAndValues));
      if (NULL == mm->keys) {
        free(mm);
        mm = NULL;
      } else {
        mm->max_keys = max_keys;
        mm->num_keys = 0;
      }
    }
  }
  
  assert(validate_multimap(mm));
  return mm;
}

int mm_insert_value(Multimap *mm, char *key, int value_num, char *value_str)
{
  assert(validate_multimap(mm));
  assert(NULL != key);
  assert(NULL != value_str);
  
  int result = -1;
  int pos;
  ValueNode *node;

  if (NULL != mm && NULL != key && NULL != value_str) {
    pos = find_key_pos(key, mm->keys, mm->num_keys);
    if (pos < 0 && mm->num_keys < mm->max_keys) {

      pos = insert_key_alphabetically(mm->keys, mm->num_keys, key);
      assert(pos >= 0 && pos < mm->max_keys);
      mm->num_keys++;
    }

    if (pos >= 0) {
      assert(pos < mm->num_keys);
      // key was either already there, or successfully added
      
      node = malloc(sizeof(ValueNode));
      if (NULL != node) {
        node->value.num = value_num;
        strncpy(node->value.str, value_str, MAX_VALUE_LENGTH);
        node->value.str[MAX_VALUE_LENGTH-1] = '\0';
        result = insert_value_by_num(&mm->keys[pos], node);
      }

      assert (result > 0);
    }
  }
  
  assert(validate_multimap(mm));
  return result;
}

int mm_count_keys(Multimap *mm)
{
  assert(validate_multimap(mm));
  
  int count = -1;
  
  if (NULL != mm) {
    count = mm->num_keys;
  }
  
  assert(count >= -1 && count <= mm->max_keys);
  return count;
}

int mm_count_values(Multimap *mm, char *key)
{
  assert(validate_multimap(mm));
  assert(NULL != key);
  
  int count = -1;
  int pos;
  
  if (NULL != mm && NULL != key) {
    count = 0;
    pos = find_key_pos(key, mm->keys, mm->num_keys);
    if (pos >= 0) {
      assert(pos < mm->num_keys);
      count = mm->keys[pos].num_values;
    }
  }
  
  assert(count >= -1);
  return count;
}

int mm_get_values(Multimap *mm, char *key, Value values[], int max_values)
{
  assert(validate_multimap(mm));
  assert(NULL != key);
  assert(NULL != values);
  assert(max_values >= 0);
  
  int count = -1;
  int pos;
  ValueNode *node;
  
  if (NULL != mm && NULL != key && NULL != values && max_values >= 0) {
    count = 0;
    pos = find_key_pos(key, mm->keys, mm->num_keys);
    if (pos >= 0) {
      assert(pos < mm->num_keys);
      node = mm->keys[pos].head;
      while (NULL != node && count < max_values) {
        values[count] = node->value;
        count++;
        node = node->next;
      }
    }
  }
  
  assert(validate_multimap(mm));
  assert(count >= -1);
  return count;
}

int mm_remove_key(Multimap *mm, char *key)
{
  assert(validate_multimap(mm));
  assert(NULL != key);
  
  int count = -1;
  int pos;
  ValueNode *curr, *next;
  
  if (NULL != mm && NULL != key) {
    count = 0;
    pos = find_key_pos(key, mm->keys, mm->num_keys);
    if (pos >= 0) {
      assert(pos < mm->num_keys);
      
      count = 0;
      
      // free the list
      curr = mm->keys[pos].head;
      while (NULL != curr) {
        next = curr->next;
        free(curr);
        curr = next;
        count++;
      }

      // move values up by one
      for (int i = pos + 1; i < mm->num_keys; i++) {
        mm->keys[i - 1] = mm->keys[i];
      }
      mm->num_keys--;
    }
  }
  
  assert(validate_multimap(mm));
  assert(count >= -1);
  return count;
}

void mm_print(Multimap *mm)
{
  assert(validate_multimap(mm));

  ValueNode *node;

  if (NULL != mm) {
    for (int i = 0; i < mm->num_keys; i++) {
      printf("[%3d] '%s' (%d):\n", i, mm->keys[i].key, mm->keys[i].num_values);
      node = mm->keys[i].head;
      while (NULL != node) {
        printf(" %9d '%s'\n", node->value.num, node->value.str);
        node = node->next;
      }
    }
  }  

  assert(validate_multimap(mm));
}

int mm_destroy(Multimap *mm)
{
  int count = -1;

  assert(validate_multimap(mm));
  
  ValueNode *node, *next;

  if (NULL != mm) {
    count = mm->num_keys;
    for (int i = 0; i < mm->num_keys; i++) {
      node = mm->keys[i].head;
      while (NULL != node) {
        next = node->next;
        free(node);
        count++;
        node = next;
      }
    }
    free(mm->keys);
    
    // set everything to zero, to help catch a dangling pointer error
    mm->num_keys = 0;
    mm->max_keys = 0;
    mm->keys = NULL;
    
    free(mm);
  }  
  
  return count;
}

static int find_key_pos(char *key, KeyAndValues *keys, int num_keys)
{
  assert(NULL != key);
  assert(NULL != keys);
  assert(num_keys >= 0);
  
  // for (int i = 0; i < num_keys; i++) {
  //   if (strcmp(key, keys[i].key) == 0) {
  //     return i;
  //   }
  // }
  
  int start = 0, end = num_keys - 1;
  int mid, comp;
  int pos = -1;
  
  while (start <= end && pos < 0) {
    mid = (end - start) / 2 + start;
    comp = strncasecmp(key, keys[mid].key, MAX_KEY_LENGTH-1);
    if (comp < 0) {
      end = mid - 1;
    } else if (comp > 0) {
      start = mid + 1;
    } else {
      pos = mid;
    }
  }
  
  return pos;
}

static int insert_key_alphabetically(KeyAndValues *keys, int keys_length, char *key) {
  assert(NULL != keys);
  assert(keys_length >= 0);
  assert(NULL != key);

  int pos = 0;
  
  while (pos < keys_length && strncasecmp(key, keys[pos].key, MAX_KEY_LENGTH-1) >= 0) {
    pos++;
  }
  
  if (pos < keys_length) {
    // shift everything over: a for loop is ok but this is more efficient
    memmove(&keys[pos+1], &keys[pos], (keys_length - pos) * sizeof(KeyAndValues));
  }
  
  strncpy(keys[pos].key, key, MAX_KEY_LENGTH);
  keys[pos].key[MAX_KEY_LENGTH-1] = '\0';
  keys[pos].num_values = 0;
  keys[pos].head = NULL;
  
  return pos;
}

static int insert_value_by_num(KeyAndValues *key, ValueNode *node) {
  assert(NULL != key);
  assert(NULL != node);

  ValueNode *curr = key->head;
  ValueNode *prev = NULL;
  
  while (NULL != curr && curr->value.num < node->value.num) {
    prev = curr;
    curr = curr->next;
  }
  
  if (NULL == prev) {
    node->next = key->head;
    key->head = node;
  } else {
    node->next = curr;
    prev->next = node;
  }
  
  key->num_values++;

  return key->num_values;
}

int mm_get_first_key(Multimap *mm, char *key, int length) {
  assert(mm != NULL);
  assert(length <= MAX_KEY_LENGTH && length > 0);
  assert(key != NULL);

  if (mm == NULL || key == NULL || length <= 0 || length > MAX_KEY_LENGTH || length < strlen(key)) {
    return -1; // Error
  }
  

  if(mm->num_keys > 0) {
    strncpy(key, mm->keys[0].key, length - 1);
    key[length - 1] = '\0';
    mm->current_count = 1;
    return 1;
  }

  
  
  return 0;
}

int mm_get_next_key(Multimap *mm, char *key, int length) {
  assert(mm != NULL);
  assert(length <= MAX_KEY_LENGTH && length > 0);
  assert(key != NULL);

  if (mm == NULL || key == NULL || length <= 0 || length > MAX_KEY_LENGTH) {
    return -1; // Error
  }
  
  if(mm->num_keys > mm->current_count) {
    strncpy(key, mm->keys[mm->current_count].key, length - 1);
    key[length - 1] = '\0';
    mm->current_count++;
    return 1;
  }

  assert(mm->current_count <= mm->num_keys);
  return 0;
}