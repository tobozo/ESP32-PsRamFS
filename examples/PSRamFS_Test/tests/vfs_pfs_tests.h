#ifdef UNIT_TESTS


#include <Arduino.h>
#include <unity.h>
#include "pfs.h"

// bring some signatures from the library
int     vfs_pfs_fopen( const char * path, int flags, int mode );
ssize_t vfs_pfs_read( int fd, void * dst, size_t size);
ssize_t vfs_pfs_write( int fd, const void * data, size_t size);
int     vfs_pfs_close(int fd);
int     vfs_pfs_fsync(int fd);
int     vfs_pfs_stat( const char * path, struct stat * st);
int     vfs_pfs_fstat( int fd, struct stat * st);
off_t   vfs_pfs_lseek(int fd, off_t offset, int mode);
int     vfs_pfs_unlink(const char *path);
int     vfs_pfs_rename( const char *src, const char *dst);
int     vfs_pfs_rmdir(const char* name);
int     vfs_pfs_mkdir(const char* name, mode_t mode);
DIR*    vfs_pfs_opendir(const char* name);
struct dirent* vfs_pfs_readdir(DIR* pdir);
int     vfs_pfs_closedir(DIR* pdir);


#include <sys/stat.h>
#include <sys/fcntl.h> // for macros AT_FDCWD / EBADF
#include <errno.h> // for 'errno' support
#include <unistd.h>
#include <esp_err.h>


#define TEST_ESP_OK(rc) TEST_ASSERT_EQUAL_HEX32(ESP_OK, rc)
#define TEST_ESP_ERR(err, rc) TEST_ASSERT_EQUAL_HEX32(err, rc)
#define pfs_partition_label "psram"
#define pfs_base_path "/psram"

static const char pfs_test_partition_label[] = pfs_partition_label;
static const char pfs_test_hello_str[]       = "Hello, World!\n";
static const char pfs_test_filename[]        = pfs_base_path "/hello.txt";


static void test_setup(void)
{
  esp_vfs_pfs_format(pfs_test_partition_label);

  const esp_vfs_pfs_conf_t conf = {
    .base_path = pfs_base_path,
    .partition_label = pfs_test_partition_label,
    .format_if_mount_failed = true
  };
  TEST_ESP_OK(esp_vfs_pfs_register(&conf));
  TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
  ESP_LOGD(TAG, "Test setup complete.");
}



static void test_teardown(void)
{
  TEST_ESP_OK(esp_vfs_pfs_unregister(pfs_base_path));
  TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
  ESP_LOGD(TAG, "Test teardown complete.");
}



static void test_pfs_create_file_with_text(const char* name, const char* text)
{
  ESP_LOGD(TAG, "Writing to \"%s\"", name);
  FILE* f = fopen(name, "wb");
  TEST_ASSERT_NOT_NULL(f);
  TEST_ASSERT_TRUE(fputs(text, f) != EOF);
  TEST_ASSERT_EQUAL(0, fclose(f));
}


static void test_setup_teardown(void)
{
  test_setup();
  test_teardown();
}


static void test_can_format_mounted_partition(void)
{
  test_setup();
  test_pfs_create_file_with_text(pfs_test_filename, pfs_test_hello_str);
  ESP_LOGD(TAG, "Deleting \"%s\" via formatting fs.", pfs_test_filename);
  esp_vfs_pfs_format( pfs_test_partition_label );
  FILE* f = fopen(pfs_test_filename, "r");
  TEST_ASSERT_NULL(f);
  test_teardown();
}


/*
static void test_ftell(void)
{
  test_setup();
  test_pfs_create_file_with_text(pfs_test_filename, pfs_test_hello_str);

  FILE *fp = fopen(pfs_test_filename, "a+");
  TEST_ASSERT_NOT_NULL(fp);

  int file_len = strlen(pfs_test_hello_str);

  TEST_ASSERT_TRUE(fputs(pfs_test_hello_str, fp) != EOF);

  file_len += strlen(pfs_test_hello_str);

  long off = ftell(fp);

  fclose(fp);

  ESP_LOGD(TAG, "ftell() returns %ld when %d is expected", off, file_len );
  TEST_ASSERT_TRUE(off == file_len);

  test_teardown();
}



static void stat_check(const char *path, struct stat *st)
{
  TEST_ASSERT_EQUAL(stat (path, st), 0);
}



static void fstat_check(int fd, struct stat *st)
{
  // Test for invalid fstat input (BZ #27559).
  TEST_ASSERT_EQUAL(fstat (AT_FDCWD, st), -1);
  TEST_ASSERT_EQUAL(errno, EBADF);
  TEST_ASSERT_EQUAL(fstat (fd, st), 0);
}

static void compare_stat( struct stat *stx, struct stat *st )
{
  ESP_LOGD(TAG, "Comparing st_size values (%d vs %d).", stx->st_size, st->st_size);
  TEST_ASSERT_EQUAL(stx->st_size, st->st_size);

  if( stx->st_mode == 8192 ) {
    ESP_LOGD(TAG, " ---> st_gid values (%d vs %d).", stx->st_gid, st->st_gid);
    ESP_LOGD(TAG, " ---> st_mode values (%d vs %d).", stx->st_mode, st->st_mode);
  } else {
    ESP_LOGD(TAG, "Comparing st_mode values (%d vs %d).", stx->st_mode, st->st_mode);
    TEST_ASSERT_EQUAL(stx->st_mode, st->st_mode);
  }

  ESP_LOGD(TAG, "Comparing st_ino values (%d vs %d).", stx->st_ino, st->st_ino);
  TEST_ASSERT_EQUAL(stx->st_ino, st->st_ino);

  ESP_LOGD(TAG, "Comparing st_nlink values (%d vs %d).", stx->st_nlink, st->st_nlink);
  TEST_ASSERT_EQUAL(stx->st_nlink, st->st_nlink);
  ESP_LOGD(TAG, "Comparing st_uid values (%d vs %d).", stx->st_uid, st->st_uid);
  TEST_ASSERT_EQUAL(stx->st_uid, st->st_uid);
  //ESP_LOGD(TAG, "Comparing st_gid values (%d vs %d).", stx->st_gid, st->st_gid);
  //TEST_ASSERT_EQUAL(stx->st_gid, st->st_gid);
  ESP_LOGD(TAG, "Comparing st_blksize values (%lu vs %lu).", stx->st_blksize, st->st_blksize);
  TEST_ASSERT_EQUAL(stx->st_blksize, st->st_blksize);
  ESP_LOGD(TAG, "Comparing st_blocks values (%lu vs %lu).", stx->st_blocks, st->st_blocks);
  TEST_ASSERT_EQUAL(stx->st_blocks, st->st_blocks);
}


static void test_stat_fstat()
{
  test_setup();
  test_pfs_create_file_with_text(pfs_test_filename, pfs_test_hello_str);

  struct stat stx;
  struct stat st;

  ESP_LOGD(TAG, "**** Testing stat() return value.");
  stat_check( pfs_base_path "/hello.txt", &stx );
  ESP_LOGD(TAG, "[OK]\n");
  ESP_LOGD(TAG, "**** Testing vfs_pfs_stat() return value.");
  TEST_ASSERT_EQUAL(vfs_pfs_stat( "/hello.txt", &st),  0);
  ESP_LOGD(TAG, "[OK]\n");

  ESP_LOGD(TAG, "#### Comparing stat() with vfs_pfs_stat()");

  compare_stat(&stx, &st);

  ESP_LOGD(TAG, "**** Testing fstat() return value.");
  fstat_check(0, &stx );
  ESP_LOGD(TAG, "[OK]\n");

  ESP_LOGD(TAG, "**** Testing vfs_pfs_fstat() return value.");
  TEST_ASSERT_EQUAL(vfs_pfs_fstat(0, &st),  0);
  ESP_LOGD(TAG, "[OK]\n");

  ESP_LOGD(TAG, "#### Comparing fstat() with vfs_pfs_fstat()");

  compare_stat(&stx, &st);

  ESP_LOGD(TAG, "[OK]");

}
*/

#endif
