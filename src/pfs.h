/*\

  MIT License

  Copyright (c) 2021-now tobozo

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

\*/

#ifndef _PFS_H_
#define _PFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include "esp_heap_caps.h"

// Configuration structure for esp_vfs_pfs_register.
typedef struct
{
  const char *base_path;            /**< Mounting point. */
  const char *partition_label;      /**< Label of partition to use. */
  uint8_t format_if_mount_failed:1; /**< Ignored but kept for confusion. */
  uint8_t dont_mount:1;             /**< Also ignored, how exciting! */
} esp_vfs_pfs_conf_t;


// File structure for pfs
typedef struct _pfs_file_t
{
  int      file_id; // file descriptor
  char*    name;    // file path
  char*    bytes;   // data
  uint32_t size;    // number of bytes in data
  uint32_t memsize; // size of allocated memory (hopefully more than size)
  uint32_t index;   // read cursor position
  int      dir_id;  // parent directory
  //int      next_file_id; // id of the next file in directory if any
  //uint32_t flags;   // file flags (not used yet)
} pfs_file_t;

// Directory structure for pfs
typedef struct _pfs_dir_t
{
  uint16_t dd_vfs_idx; /*!< VFS index, not to be used by applications */
  uint16_t dd_rsv;     /*!< field reserved for future extension */
  int    dir_id; // dir descriptor
  char * name;   // dir path
  int    pos;    // position while reading dir (reset by opendir)
  int    itemscount;
  struct _pfs_dir_t* parent_dir; // parent directory if any
  struct dirent ** items; // collection of items (file or dir) in that directory
} pfs_dir_t;

// Seek modes
typedef enum
{
  pfs_seek_set = 0,
  pfs_seek_cur = 1,
  pfs_seek_end = 2
} pfs_seek_mode;

// File open flags
typedef enum  {
  // open flags
  PFS_O_RDONLY = 1,         // Open a file as read only
  PFS_O_WRONLY = 2,         // Open a file as write only
  PFS_O_RDWR   = 3,         // Open a file as read and write
  PFS_O_CREAT  = 0x0100,    // Create a file if it does not exist
  PFS_O_EXCL   = 0x0200,    // Fail if a file already exists
  PFS_O_TRUNC  = 0x0400,    // Truncate the existing file to zero size
  PFS_O_APPEND = 0x0800,    // Move to end of file on every write

  // internally used flags
  PFS_F_DIRTY   = 0x010000, // File does not match storage
  PFS_F_WRITING = 0x020000, // File has been written since last flush
  PFS_F_READING = 0x040000, // File has been read since last flush
  PFS_F_ERRED   = 0x080000, // An error occured during write
  PFS_F_INLINE  = 0x100000, // Currently inlined in directory entry
  PFS_F_OPENED  = 0x200000, // File has been opened

} pfs_open_flags;


// those are exposed to the fs::PSRamFS layer

pfs_file_t** pfs_get_files(); // returns pointer to the files array
pfs_dir_t**  pfs_get_dirs();  // returns pointer to the directories array
int          pfs_get_max_items(); // how many items in the files/directories arrays (same for both)
void         pfs_set_max_items(size_t max_items); // applies to both files and directories
size_t       pfs_get_block_size();
void         pfs_set_block_size(size_t block_size); // smaller value = more calls to realloc()
size_t       pfs_get_partition_size();
void         pfs_set_partition_size( size_t size );
bool         pfs_get_psram();
void         pfs_set_psram( bool use );
size_t       pfs_used_bytes();
void         pfs_clean_files();
void         pfs_free();
void         pfs_deinit();

esp_err_t    esp_vfs_pfs_register(const esp_vfs_pfs_conf_t *conf);
esp_err_t    esp_vfs_pfs_format(const char* partition_label);
esp_err_t    esp_vfs_pfs_info(const char* partition_label, size_t *total_bytes, size_t *used_bytes);
esp_err_t    esp_vfs_pfs_unregister(const char* base_path );

#ifdef __cplusplus
}
#endif

#endif
