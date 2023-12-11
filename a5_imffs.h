#ifndef _A5_IMMFS
#define _A5_IMMFS

#include <stdint.h>

// Using a typedef'd pointer to the IMFFS struct, for a change
typedef struct IMFFS *IMFFSPtr;

// All functions should print a message in the event of any error, and also return an error code as follows:
//  IMFFS_OK is zero and means that the function was successful;
//  IMFFS_ERROR means that something went wrong and the operation couldn't complete but it was recoverable (e.g. running out of space on the device while saving a file, or renaming a file that doesn't exist or to a filename that already exists).
//  IMFFS_FATAL means that a fatal error occured and the program cannot continue (e.g. running out of memory during malloc);
//  IMFFS_INVALID means that the function was called incorrectly (e.g. a NULL parameter where a string was required);
//  IMFFS_NOT_IMPLEMENTED means that the function is not implemented.
// Not all functions need to use all of these return codes. The exact choice of what value is most appropriate for any given situation is up to you.

typedef enum {
  IMFFS_OK = 0,
  IMFFS_ERROR = 1,
  IMFFS_FATAL = 2,
  IMFFS_INVALID = 3,
  IMFFS_NOT_IMPLEMENTED = 4
} IMFFSResult;

// this function will create the filesystem with the given number of blocks;
// it will modify the fs parameter to point to the new file system or set it
// to NULL if something went wrong (fs is a pointer to a pointer)
IMFFSResult imffs_create(uint32_t block_count, IMFFSPtr *fs);

// save diskfile imffsfile copy from your system to IMFFS
IMFFSResult imffs_save(IMFFSPtr fs, char *diskfile, char *imffsfile);

// load imffsfile diskfile copy from IMFFS to your system
IMFFSResult imffs_load(IMFFSPtr fs, char *imffsfile, char *diskfile);

// delete imffsfile remove the IMFFS file from the system, allowing the blocks to be used for other files
IMFFSResult imffs_delete(IMFFSPtr fs, char *imffsfile);

// rename imffsold imffsnew rename the IMFFS file from imffsold to imffsnew, keeping all of the data intact
IMFFSResult imffs_rename(IMFFSPtr fs, char *imffsold, char *imffsnew);

// dir will list all of the files and the number of bytes they occupy
IMFFSResult imffs_dir(IMFFSPtr fs);

// fulldir is like "dir" except it shows a the files and details about all of the chunks they are stored in (where, and how big)
IMFFSResult imffs_fulldir(IMFFSPtr fs);

// defrag will defragment the filesystem: if you haven't implemented it, have it print "feature not implemented" and return IMFFS_NOT_IMPLEMENTED
IMFFSResult imffs_defrag(IMFFSPtr fs);

// quit will quit the program: clean up the data structures
IMFFSResult imffs_destroy(IMFFSPtr fs);

#endif