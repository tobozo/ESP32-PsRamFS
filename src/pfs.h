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


// Configuration structure for esp_vfs_pfs_register.
typedef struct
{
  const char *base_path;            /**< Mounting point. */
  const char *partition_label;      /**< Label of partition to use. */
  uint8_t format_if_mount_failed:1; /**< Format the file system if it fails to mount. */
  uint8_t dont_mount:1;             /**< Don't attempt to mount or format. Overrides format_if_mount_failed */
} esp_vfs_pfs_conf_t;


// File structure for pfs
typedef struct
{
  int file_id;
  char*    name;
  char*    bytes;
  uint32_t size;
  uint32_t memsize;
  uint32_t index;
  //uint32_t flags;
} pfs_file_t;


// Directory structure for pfs
typedef struct
{
  int dir_id;
  char * name;
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
/*
  // internally used flags
  PFS_F_DIRTY   = 0x010000, // File does not match storage
  PFS_F_WRITING = 0x020000, // File has been written since last flush
  PFS_F_READING = 0x040000, // File has been read since last flush
  PFS_F_ERRED   = 0x080000, // An error occured during write
  PFS_F_INLINE  = 0x100000, // Currently inlined in directory entry
  PFS_F_OPENED  = 0x200000, // File has been opened
*/
} pfs_open_flags;



pfs_file_t ** pfs_get_files();
pfs_dir_t  ** pfs_get_dirs();
int         pfs_get_max_items();
void        pfs_set_max_items(size_t max_items);
int         pfs_get_block_size();
void        pfs_set_block_size(size_t block_size);
void        pfs_free();
void        pfs_clean_files();
//int         pfs_stat( const char * path, const void *_stat );
int         pfs_stat( const char * path, struct stat * stat_ );
pfs_file_t* pfs_fopen( const char * path, const char* mode );
size_t      pfs_fread( uint8_t *buf, size_t size, size_t count, pfs_file_t * stream );
size_t      pfs_fwrite( const uint8_t *buf, size_t size, size_t count, pfs_file_t * stream);
int         pfs_fflush(pfs_file_t * stream);
int         pfs_fseek( pfs_file_t * stream, long offset, pfs_seek_mode mode );
size_t      pfs_ftell( pfs_file_t * stream );
void        pfs_fclose( pfs_file_t * stream );
int         pfs_unlink( const char * path );
int         pfs_rename( const char * from, const char * to );
pfs_dir_t*  pfs_opendir( const char * path );
int         pfs_mkdir( const char* path );
struct dirent * pfs_readdir( pfs_dir_t * dir );
void        pfs_closedir( pfs_dir_t * dir );
void        pfs_rewinddir( pfs_dir_t * dir );
void        pfs_set_psram( bool use );
bool        pfs_get_psram();
size_t      pfs_used_bytes();
void        pfs_set_partition_size( size_t size );
size_t      pfs_get_partition_size();
esp_err_t esp_vfs_pfs_register(const esp_vfs_pfs_conf_t *conf);

#ifdef __cplusplus
}
#endif

#endif
