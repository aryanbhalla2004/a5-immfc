#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "a4_tests.h"
#include "a5_multimap.h"
#include "a5_imffs.h"


void test_valid_test_cases() {
  IMFFSPtr fs;
  printf("\n*** Valid cases:\n\n");

  VERIFY_INT(IMFFS_OK, imffs_create(5, &fs));
  VERIFY_INT(IMFFS_OK, imffs_save(fs, "test/1", "a"));
  VERIFY_INT(IMFFS_OK, imffs_save(fs, "test/4", "b"));

  printf("\n*** File exist:\n\n");
  VERIFY_INT(IMFFS_OK, imffs_load(fs, "a", "1out"));

  printf("\n*** File doesn't exist :\n\n");
  VERIFY_INT(IMFFS_ERROR, imffs_load(fs, "c", "2out"));

  printf("\n*** No Space Left :\n\n");
  VERIFY_INT(IMFFS_ERROR, imffs_save(fs, "test/4", "c"));
  VERIFY_INT(IMFFS_ERROR, imffs_save(fs, "test/4", "d"));

  VERIFY_INT(IMFFS_OK, imffs_destroy(fs));
}


void test_invalid() {
  printf("\n*** Testing Invalid Invalid cases:\n\n");

  VERIFY_INT(IMFFS_INVALID, imffs_create(0, NULL));
  VERIFY_INT(IMFFS_INVALID, imffs_create(0, NULL));

  VERIFY_INT(IMFFS_INVALID, imffs_save(NULL, NULL, NULL));
  VERIFY_INT(IMFFS_INVALID, imffs_load(NULL, NULL, NULL));
  VERIFY_INT(IMFFS_INVALID, imffs_delete(NULL, NULL));
  VERIFY_INT(IMFFS_INVALID, imffs_rename(NULL, NULL, NULL));
  VERIFY_INT(IMFFS_INVALID, imffs_destroy(NULL));
}

int main() {
  printf("*** Starting tests...\n");
  test_valid_test_cases();
#ifdef NDEBUG
  test_invalid();
#endif
  
  if (0 == Tests_Failed) {
    printf("\nAll %d tests passed.\n", Tests_Passed);
  } else {
    printf("\nFAILED %d of %d tests.\n", Tests_Failed, Tests_Failed+Tests_Passed);
  }
  
  printf("\n*** Tests complete.\n");  
  return 0;
} 