

/*
String STR_TO_TEST;

void setUp(void) {
    // set stuff up here
    STR_TO_TEST = "Hello, world!";
}

void tearDown(void) {
    // clean stuff up here
    STR_TO_TEST = "";
}

void test_string_concat(void) {
    String hello = "Hello, ";
    String world = "world!";
    TEST_ASSERT_EQUAL_STRING(STR_TO_TEST.c_str(), (hello + world).c_str());
}

void test_string_substring(void) {
    TEST_ASSERT_EQUAL_STRING("Hello", STR_TO_TEST.substring(0, 5).c_str());
}

void test_string_index_of(void) {
    TEST_ASSERT_EQUAL(7, STR_TO_TEST.indexOf('w'));
}

void test_string_equal_ignore_case(void) {
    TEST_ASSERT_TRUE(STR_TO_TEST.equalsIgnoreCase("HELLO, WORLD!"));
}

void test_string_to_upper_case(void) {
    STR_TO_TEST.toUpperCase();
    TEST_ASSERT_EQUAL_STRING("HELLO, WORLD!", STR_TO_TEST.c_str());
}

void test_string_replace(void) {
    STR_TO_TEST.replace('!', '?');
    TEST_ASSERT_EQUAL_STRING("Hello, world?", STR_TO_TEST.c_str());
}
*/


#include <Arduino.h>
#include <unity.h>
#include "pfs.h"

#include <sys/stat.h>
#include "fcntl.h" // for macros AT_FDCWD / EBADF
#include "errno.h" // for 'errno' support

#include <unistd.h>
#include <esp_err.h>
// fuck you esp-idf for hiding that deep in your core
#define TEST_ESP_OK(rc) TEST_ASSERT_EQUAL_HEX32(ESP_OK, rc)
#define TEST_ESP_ERR(err, rc) TEST_ASSERT_EQUAL_HEX32(err, rc)

#define pfs_partition_label "psram"
#define pfs_base_path "/psram"

static const char pfs_test_partition_label[] = pfs_partition_label;
static const char pfs_test_hello_str[]       = "Hello, World!\n";
static const char pfs_test_filename[]        = pfs_base_path "/hello.txt";



static void test_setup(void) {

  if (!psramInit() ){
    log_w("No psram found, using heap");
    pfs_set_psram( false );
    pfs_set_partition_size( 100*1024 /*0.25 * ESP.getFreeHeap()*/ );
  } else {
    pfs_set_psram( true );
    pfs_set_partition_size( FPSRAM_PARTITION_SIZE * ESP.getFreePsram() );
  }

  esp_vfs_pfs_format(pfs_test_partition_label);

  const esp_vfs_pfs_conf_t conf = {
    .base_path = pfs_base_path,
    .partition_label = pfs_test_partition_label,
    .format_if_mount_failed = true
  };
  TEST_ESP_OK(esp_vfs_pfs_register(&conf));
  TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
  printf("Test setup complete.\n");
}



static void test_teardown(void){
  TEST_ESP_OK(esp_vfs_pfs_unregister(pfs_base_path));
  TEST_ASSERT_TRUE( heap_caps_check_integrity_all(true) );
  printf("Test teardown complete.\n");
}



static void test_pfs_create_file_with_text(const char* name, const char* text)
{
  printf("Writing to \"%s\"\n", name);
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
  printf("Deleting \"%s\" via formatting fs.\n", pfs_test_filename);
  esp_vfs_pfs_format( pfs_test_partition_label );
  FILE* f = fopen(pfs_test_filename, "r");
  TEST_ASSERT_NULL(f);
  test_teardown();
}



static void test_ftell(void)
{
  test_setup();
  test_pfs_create_file_with_text(pfs_test_filename, pfs_test_hello_str);

  FILE *fp = fopen(pfs_test_filename, "a+");
  TEST_ASSERT_NOT_NULL(fp);

  //char char_buf[4];
  //void *ret = fgets(char_buf, 4, fp);
  //TEST_ASSERT_NOT_NULL(ret);

  int file_len = strlen (pfs_test_hello_str);

  TEST_ASSERT_TRUE(fputs(pfs_test_hello_str, fp) != EOF);

  file_len += strlen (pfs_test_hello_str);

  long off = ftell(fp);

  printf("ftell() returns %ld when %d is expected\n", off, file_len );
  TEST_ASSERT_TRUE(off == file_len);

  test_teardown();
}



static void stat_check(const char *path, struct stat *st)
{
  TEST_ASSERT_EQUAL(stat (path, st), 0);
}



static void fstat_check(int fd, struct stat *st)
{
  /* Test for invalid fstat input (BZ #27559).  */
  TEST_ASSERT_EQUAL(fstat (AT_FDCWD, st), -1);
  TEST_ASSERT_EQUAL(errno, EBADF);
  TEST_ASSERT_EQUAL(fstat (fd, st), 0);
}

static void compare_stat( struct stat *stx, struct stat *st )
{
  printf("Comparing st_ino values (%d vs %d).\n", stx->st_ino, st->st_ino);
  TEST_ASSERT_EQUAL(stx->st_ino, st->st_ino);
  printf("Comparing st_mode values (%d vs %d).\n", stx->st_mode, st->st_mode);
  TEST_ASSERT_EQUAL(stx->st_mode, st->st_mode);
  printf("Comparing st_nlink values (%d vs %d).\n", stx->st_nlink, st->st_nlink);
  TEST_ASSERT_EQUAL(stx->st_nlink, st->st_nlink);
  printf("Comparing st_uid values (%d vs %d).\n", stx->st_uid, st->st_uid);
  TEST_ASSERT_EQUAL(stx->st_uid, st->st_uid);
  printf("Comparing st_gid values (%d vs %d).\n", stx->st_gid, st->st_gid);
  TEST_ASSERT_EQUAL(stx->st_gid, st->st_gid);
  printf("Comparing st_blksize values (%lu vs %lu).\n", stx->st_blksize, st->st_blksize);
  TEST_ASSERT_EQUAL(stx->st_blksize, st->st_blksize);
  printf("Comparing st_blocks values (%lu vs %lu).\n", stx->st_blocks, st->st_blocks);
  TEST_ASSERT_EQUAL(stx->st_blocks, st->st_blocks);
}


// struct stat
// {
//   dev_t         st_dev;      /* device */
//   ino_t         st_ino;      /* inode */
//   umode_t       st_mode;     /* protection */
//   nlink_t       st_nlink;    /* number of hard links */
//   uid_t         st_uid;      /* user ID of owner */
//   gid_t         st_gid;      /* group ID of owner */
//   dev_t         st_rdev;     /* device type (if inode device) */
//   off_t         st_size;     /* total size, in bytes */
//   unsigned long st_blksize;  /* blocksize for filesystem I/O */
//   unsigned long st_blocks;   /* number of blocks allocated */
//   time_t        st_atime;    /* time of last access */
//   time_t        st_mtime;    /* time of last modification */
//   time_t        st_ctime;    /* time of last change */
// };


static void test_stat_fstat()
{
  test_setup();
  test_pfs_create_file_with_text(pfs_test_filename, pfs_test_hello_str);

  struct stat stx;
  struct stat st;

  printf("**** Testing stat() return value.\n");
  stat_check( pfs_base_path "/hello.txt", &stx );
  printf("[OK]\n");
  printf("**** Testing vfs_pfs_stat() return value.\n");
  TEST_ASSERT_EQUAL(vfs_pfs_stat( "/hello.txt", &st),  0);
  printf("[OK]\n");

  printf("#### Comparing stat() with vfs_pfs_stat()\n");

  compare_stat(&stx, &st);

  //FILE *fp = fopen(pfs_test_filename, "r");
  //TEST_ASSERT_NOT_NULL(fp);

  printf("**** Testing fstat() return value.\n");
  fstat_check(0, &stx );
  printf("[OK]\n");

  printf("**** Testing vfs_pfs_fstat() return value.\n");
  TEST_ASSERT_EQUAL(vfs_pfs_fstat(0, &st),  0);
  printf("[OK]\n");

  //fclose(fp);

  printf("#### Comparing fstat() with vfs_pfs_fstat()\n");

  compare_stat(&stx, &st);

  printf("[OK]\n");

}



#if 0

/* Verify that ftell returns the correct value after a read and a write on a
   file opened in a+ mode.
   Copyright (C) 2014-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>

/* data points to either char_data or wide_data, depending on whether we're
   testing regular file mode or wide mode respectively.  Similarly,
   fputs_func points to either fputs or fputws.  data_len keeps track of the
   length of the current data and file_len maintains the current file
   length.  */
#define BUF_LEN 4
static void *buf;
static char char_buf[BUF_LEN];
static wchar_t wide_buf[BUF_LEN];
static const void *data;
static const char *char_data = "abcdefghijklmnopqrstuvwxyz";
static const wchar_t *wide_data = L"abcdefghijklmnopqrstuvwxyz";
static size_t data_len;
static size_t file_len;

typedef int (*fputs_func_t) (const void *data, FILE *fp);
fputs_func_t fputs_func;

typedef void *(*fgets_func_t) (void *s, int size, FILE *stream);
fgets_func_t fgets_func;

static int do_test (void);

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"

static FILE * init_file (const char *filename) {
  FILE *fp = fopen (filename, "w");
  if (fp == NULL) {
    printf ("fopen: %m\n");
    return NULL;
  }
  int written = fputs_func (data, fp);
  if (written == EOF) {
    printf ("fputs failed to write data\n");
    fclose (fp);
    return NULL;
  }
  file_len = data_len;
  fclose (fp);
  fp = fopen (filename, "a+");
  if (fp == NULL) {
    printf ("fopen(a+): %m\n");
    return NULL;
  }
  return fp;
}

static int do_one_test (const char *filename) {
  FILE *fp = init_file (filename);
  if (fp == NULL)
    return 1;
  void *ret = fgets_func (buf, BUF_LEN, fp);
  if (ret == NULL) {
    printf ("read failed: %m\n");
    fclose (fp);
    return 1;
  }
  int written = fputs_func (data, fp);
  if (written == EOF) {
    printf ("fputs failed to write data\n");
    fclose (fp);
    return 1;
  }
  file_len += data_len;
  long off = ftell (fp);
  if (off != file_len) {
    printf ("Incorrect offset %ld, expected %zu\n", off, file_len);
    fclose (fp);
    return 1;
  } else
    printf ("Correct offset %ld after write.\n", off);
  return 0;
}

/* Run the tests for regular files and wide mode files.  */
static int do_test (void) {
  int ret = 0;
  char *filename;
  int fd = create_temp_file ("tst-ftell-append-tmp.", &filename);
  if (fd == -1) {
    printf ("create_temp_file: %m\n");
    return 1;
  }
  close (fd);
  /* Tests for regular files.  */
  puts ("Regular mode:");
  fputs_func = (fputs_func_t) fputs;
  fgets_func = (fgets_func_t) fgets;
  data = char_data;
  buf = char_buf;
  data_len = strlen (char_data);
  ret |= do_one_test (filename);
  /* Tests for wide files.  */
  puts ("Wide mode:");
  if (setlocale (LC_ALL, "en_US.UTF-8") == NULL)  {
    printf ("Cannot set en_US.UTF-8 locale.\n");
    return 1;
  }
  fputs_func = (fputs_func_t) fputws;
  fgets_func = (fgets_func_t) fgetws;
  data = wide_data;
  buf = wide_buf;
  data_len = wcslen (wide_data);
  ret |= do_one_test (filename);
  return ret;
}

#endif
