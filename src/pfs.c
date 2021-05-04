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


#if __has_include("esp_arduino_version.h")
#include "esp_arduino_version.h"
#endif

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#endif

#include "esp_log.h"


#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/semphr.h"
#include <unistd.h>
#include <dirent.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/lock.h>
#include <sys/param.h>

#include <string.h>

#include "esp_heap_caps.h"
#include "esp32-hal-log.h"

#define realloc(p,s)    heap_caps_realloc(p, s, MALLOC_CAP_8BIT)
#define ps_malloc(p)    heap_caps_malloc(p, MALLOC_CAP_SPIRAM)
#define ps_realloc(p,s) heap_caps_realloc(p, s, MALLOC_CAP_SPIRAM)
#define ps_calloc(p, s) heap_caps_calloc(p, s, MALLOC_CAP_SPIRAM)
#define ALLOC_BLOCK_SIZE 4096
#define MAX_PSRAM_FILES 256

#include "pfs.h"
#include "esp_vfs.h"

static const char TAG[] = "esp_psramfs";

pfs_file_t ** pfs_files;
pfs_dir_t  ** pfs_dirs;

int pfs_files_set = -1;
int pfs_dirs_set = -1;

pfs_file_t ** pfs_get_files();
pfs_dir_t  ** pfs_get_dirs();
int         pfs_max_items();
void        pfs_init_files();
void        pfs_init_dirs();
int         pfs_next_file_avail();
int         pfs_next_dir_avail();
int         pfs_find_file( const char* path );
int         pfs_find_dir( const char* path );
int         pfs_stat( const char * path, const void *_stat );
pfs_file_t* pfs_fopen( const char * path, const char* mode );
size_t      pfs_fread( uint8_t *buf, size_t size, size_t count, pfs_file_t * stream );
size_t      pfs_fwrite( const uint8_t *buf, size_t size, size_t count, pfs_file_t * stream);
int         pfs_fflush(pfs_file_t * stream);
int         pfs_fseek( pfs_file_t * stream, long offset, pfs_seek_mode mode );
size_t      pfs_ftell( pfs_file_t * stream );
void        pfs_fclose( pfs_file_t * stream );
int         pfs_unlink( const char * path );
void        pfs_free();
void        pfs_clean_files();
int         pfs_rename( const char * from, const char * to );
pfs_dir_t*  pfs_opendir( const char * path );
int         pfs_mkdir( const char* path );
int         pfs_rmdir( const char* path );
struct dirent * pfs_readdir( pfs_dir_t * dir );
void        pfs_closedir( pfs_dir_t * dir );
void        pfs_rewinddir( pfs_dir_t * dir );

static char pfs_flag[3] = {0,0,0};
char *pfs_flags_conv(int m);

static int     vfs_pfs_fopen( const char * path, int flags, int mode );
static ssize_t vfs_pfs_read( int fd, void * dst, size_t size);
static ssize_t vfs_pfs_write( int fd, const void * data, size_t size);
static int     vfs_pfs_close(int fd);
static int     vfs_pfs_fsync(int fd);
static int     vfs_pfs_stat( const char * path, struct stat * st);
static int     vfs_pfs_fstat( int fd, struct stat * st);
static off_t   vfs_pfs_lseek(int fd, off_t offset, int mode);
static int     vfs_pfs_unlink(const char *path);
static int     vfs_pfs_rename( const char *src, const char *dst);
static int     vfs_pfs_rmdir(const char* name);
static int     vfs_pfs_mkdir(const char* name, mode_t mode);
static DIR*    vfs_pfs_opendir(const char* name);
static struct dirent* vfs_pfs_readdir(DIR* pdir);
static int     vfs_pfs_closedir(DIR* pdir);

esp_err_t  esp_vfs_pfs_register(const esp_vfs_pfs_conf_t* conf);



pfs_file_t ** pfs_get_files() { return pfs_files; }
pfs_dir_t  ** pfs_get_dirs()  { return pfs_dirs;  }

int pfs_max_items()
{
  return MAX_PSRAM_FILES;
}


void pfs_init_files()
{
  pfs_files = (pfs_file_t**)ps_calloc( MAX_PSRAM_FILES, sizeof( pfs_file_t* ) );
  if( pfs_files == NULL ) {
    log_e("Unable to init psram fs, halting");
    while(1);
  }
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    pfs_files[i] = (pfs_file_t*)ps_calloc( 1, sizeof( pfs_file_t ) );
    if( pfs_files[i] == NULL ) {
      log_e("Unable to init psram fs, halting");
      while(1);
    }
  }
  log_d("Psram init files OK");
}


void pfs_init_dirs()
{
  pfs_dirs = (pfs_dir_t **)ps_calloc( MAX_PSRAM_FILES, sizeof( pfs_dir_t* ) );
  if( pfs_dirs == NULL ) {
    log_e("Unable to init psram fs, halting");
    while(1);
  }
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    pfs_dirs[i] = (pfs_dir_t*)ps_calloc( 1, sizeof( pfs_dir_t ) );
    if( pfs_dirs[i] == NULL ) {
      log_e("Unable to init psram fs, halting");
      while(1);
    }
  }
  log_d("Psram init dirs OK");
}


int pfs_next_file_avail()
{
  int res = 0;
  if( pfs_files != NULL ) {
    for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
      if( pfs_files[i]->name == NULL ) {
        log_v("File Slot %d out of %d is free [r]", i, MAX_PSRAM_FILES );
        return i;
      }
    }
    log_e("Too many files created.");
  } else {
    log_e("No allocated space for files");
  }
  return res;
}


int pfs_next_dir_avail()
{
  int res = 0;
  if( pfs_dirs != NULL ) {
    for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
      if( pfs_dirs[i]->name == NULL ) {
        log_v("Dir Slot %d out of %d is free [r]", i, MAX_PSRAM_FILES );
        return i;
      }
    }
  }
  log_e("Too many dirs created, halting");
  while(1);
  return res;
}


int pfs_find_file( const char* path )
{
  if( pfs_files != NULL ) {
    for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
      if( pfs_files[i]->name == NULL ) continue;
      if( strcmp( path, pfs_files[i]->name ) == 0 ) {
        return i;
      }
    }
  } else {
    pfs_init_files();
  }
  return -1;
}


int pfs_find_dir( const char* path )
{
  if( pfs_dirs != NULL ) {
    unsigned long entitycount = 0;
    for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
      if( pfs_dirs[i]->name == NULL ) continue;
      if( strcmp( path, pfs_dirs[i]->name ) == 0 ) {
        return i;
      }
    }
  } else {
    pfs_init_dirs();
  }
  return -1;
}


int pfs_stat( const char * path, const void *_stat )
{
  struct pfs_stat_t* stat_;
  stat_ = (pfs_stat_t*)_stat;
  stat_->st_mode = DT_UNKNOWN;

  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    stat_->st_size = pfs_files[file_id]->size;
    stat_->st_mode = DT_REG;
    stat_->st_name = pfs_files[file_id]->name;
    log_v("stating for DT_REG(%s) success", path );
    return 0;
  } else {
    log_v("stating for DT_REG(%s) no pass", path );
  }

  int dir_id = pfs_find_dir( path );
  if( dir_id > -1 ) {
    stat_->st_mode = DT_DIR;
    log_v("stating for DT_DIR(%s) success", path );
    return 0;
  } else {
    log_v("stating for DT_DIR(%s) no pass", path );
  }

  return 1;
}


pfs_file_t* pfs_fopen( const char * path, const char* mode )
{

  if( path == NULL ) {
    log_e("Invalid path");
    return NULL;
  } else {
    log_v("valid path :%s", path );
  }

  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    // existing file
    if( mode ) {
      switch( mode[0] ) {
        case 'a': // seek end
          if( pfs_files[file_id]->size > 0 ) {
            pfs_files[file_id]->index = pfs_files[file_id]->size - 1;
          }
        break;
        case 'w': // truncate
          if( pfs_files[file_id]->bytes != NULL ) {
            free( pfs_files[file_id]->bytes );
            pfs_files[file_id]->bytes = NULL;
          }
          pfs_files[file_id]->index = 0;
          pfs_files[file_id]->size  = 0;
        break;
        case 'r':
          pfs_files[file_id]->index = 0;
        break;
      }
    }
    log_d( "file exists: %s (mode %s)", path, mode);
    return pfs_files[file_id];
  }

  if(mode && (mode[0] != 'r' || mode[1] == '+') ) {
    // new file, write mode
    int slotscount = 0;
    int fileslot = pfs_next_file_avail();

    if( fileslot < 0 || pfs_files[fileslot] == NULL ) {
      log_e("alloc fail!");
      return NULL;
    }
    if( pfs_files[fileslot]->name != NULL ) {
      log_e("Name from file slot #%d is now null, freeing", fileslot);
      free( pfs_files[fileslot]->name );
    }
    int pathlen = strlen(path);
    pfs_files[fileslot]->name = (char*)ps_malloc( pathlen+1 );
    memcpy( pfs_files[fileslot]->name, path, pathlen+1 );
    pfs_files[fileslot]->index = 0; // default truncate
    pfs_files[fileslot]->size  = 0;
    pfs_files[fileslot]->file_id = fileslot;

    log_d( "file created: %s (mode %s)", path, mode);
    return pfs_files[fileslot];
  }
  log_e("can't open: %s (mode %s)", path, mode);
  return NULL;
}


size_t pfs_fread( uint8_t *buf, size_t size, size_t count, pfs_file_t * stream )
{
  size_t to_read = size*count;
  if( ( stream->index + to_read ) > stream->size ) {
    log_e("Attempted to read %d out of bounds bytes at index %d of %d", to_read, stream->index, stream->size );
    return 1;
  }
  memcpy( buf, &stream->bytes[stream->index], to_read );
  if( to_read > 1 ) {
    log_v("Reading %d byte(s) at index %d of %d", to_read, stream->index, stream->size );
  } else {
    char out[2] = {0,0};
    out[0] = buf[0];
    log_v("Reading %d byte(s) at index %d of %d (%s)", to_read, stream->index, stream->size, out );
  }
  stream->index += to_read;
  return to_read;
}


size_t pfs_fwrite( const uint8_t *buf, size_t size, size_t count, pfs_file_t * stream)
{
  size_t to_write = size*count;
  if( stream->index + to_write > stream->memsize ) {
    if( stream->bytes == NULL ) {
      log_d("Allocating %d bytes to write %d bytes", ALLOC_BLOCK_SIZE, to_write );
      stream->bytes = (char*)ps_malloc( ALLOC_BLOCK_SIZE /*, sizeof(char) */);
      stream->memsize = ALLOC_BLOCK_SIZE;
    }
    while( stream->index + to_write > stream->memsize ) {
      log_d("Reallocating %d bytes to write %d bytes at index %d/%d => %d", ALLOC_BLOCK_SIZE, to_write, stream->index, stream->size, stream->index + ALLOC_BLOCK_SIZE  );
      log_d("stream->bytes = (char*)realloc( %d, %d );", stream->bytes, stream->index + ALLOC_BLOCK_SIZE  );
      stream->bytes = (char*)ps_realloc( stream->bytes, stream->memsize + ALLOC_BLOCK_SIZE  );
      stream->memsize += ALLOC_BLOCK_SIZE;
    }
  } else {
    log_v("Writing %d bytes at index %d of %d (no realloc, memsize = %d)", to_write, stream->index, stream->size, stream->memsize );
  }
  memcpy( &stream->bytes[stream->index], buf, to_write );
  stream->index += to_write;
  stream->size  += to_write;

  return to_write;
}

int pfs_fflush(pfs_file_t * stream)
{
  return 0;
}


int pfs_fseek( pfs_file_t * stream, long offset, pfs_seek_mode mode )
{
  log_v("Seeking mode #%s with offset %d", mode, offset );
  switch( mode ) {
    case pfs_seek_set:
      if( offset < stream->size ) {
        stream->index = offset;
        break;
      }
      return -1;
    case pfs_seek_cur:
      if( stream->index + offset < stream->size ) {
        stream->index += offset;
        break;
      }
      return -1;
    case pfs_seek_end:
      if( (stream->size-1) - offset > stream->size ) {
        stream->index = (stream->size-1) - offset;
      }
      return -1;
      break;
  }
  return 0;
}


size_t pfs_ftell( pfs_file_t * stream )
{
  log_d("Getting cursor for %s = %d", stream->name, stream->index+1);
  return stream->index+1;
}


void pfs_fclose( pfs_file_t * stream )
{
  if( stream->size < 128 ) {
    log_d("Closing stream %s (%d bytes) contents: %s\n", stream->name, stream->size, (const char*)stream->bytes);
  } else {
    log_d("Closing stream %s (%d bytes)\n", stream->name, stream->size );
  }
  stream->index = 0;
  return;
}


int pfs_unlink( const char * path )
{
  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    free( pfs_files[file_id]->name );
    pfs_files[file_id]->name = NULL;
    if( pfs_files[file_id]->bytes != NULL ) {
      free( pfs_files[file_id]->bytes );
      pfs_files[file_id]->bytes = NULL;
    }
    pfs_files[file_id]->size = 0;
    pfs_files[file_id]->memsize = 0;
    pfs_files[file_id]->index = 0;
    log_w("Path %s unlinked successfully", path);
    return 0;
  }
  log_w("Path %s not found, can't unlink", path);
  return 1;
}


void pfs_free()
{
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    if( pfs_files[i]->name != NULL ) {
      pfs_unlink( pfs_files[i]->name );
      free( pfs_files[i] );
    }
  }
  free( pfs_files );
}


void pfs_clean_files()
{
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    if( pfs_files[i]->name != NULL ) {
      pfs_unlink( pfs_files[i]->name );
    }
  }
}


int pfs_rename( const char * from, const char * to )
{
  int file_id = pfs_find_file( to );
  if( file_id > -1 ) {
    // destination file already exists, unlink first ?
    return -1;
  }
  file_id = pfs_find_file( from );
  if( file_id > -1 ) {
    free( pfs_files[file_id]->name );
    pfs_files[file_id]->name = (char*)ps_malloc( strlen(to)+1 );
    memcpy( pfs_files[file_id]->name, to, strlen(to)+1 );
    return 0;
  }

  int dir_id = pfs_find_dir( to );
  if( dir_id > -1 ) {
    // destination dir already exists, unlink first ?
    return -1;
  }
  dir_id = pfs_find_dir( from );
  if( dir_id > -1 ) {
    free( pfs_dirs[dir_id]->name );
    pfs_dirs[dir_id]->name = (char*)ps_malloc( strlen(to)+1 );
    memcpy( pfs_dirs[dir_id]->name, to, strlen(to)+1 );
    return 0;
  }

  return -1;
}


pfs_dir_t* pfs_opendir( const char * path )
{

  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    log_w("Can't create dir(%s) as a file already exists with the same name", path );
    return NULL;
  }

  int dir_id = pfs_find_dir( path );
  if( dir_id > -1 ) {
    // directory exists, no need to create
    return pfs_dirs[dir_id];
  }

  return NULL;
}


int pfs_mkdir( const char* path )
{
  int dir_id = pfs_find_dir( path );
  if( dir_id > -1 ) {
    // directory exists, no need to create
    return 0;
  }

  int slotscount = 0;
  int dirslot = pfs_next_dir_avail();
  pfs_dirs[dirslot]->name = (char*)ps_calloc(1, sizeof( path ) +1 );
  pfs_dirs[dirslot]->dir_id = dirslot;
  memcpy( pfs_dirs[dirslot]->name, path, sizeof( path ) +1 );
  if( pfs_dirs[dirslot] != NULL ) {
    return 0;
  }
  return 1;
}


int pfs_rmdir( const char* path )
{
  int dir_id = pfs_find_dir( path );
  if( dir_id < 0 ) {
    // directory exists, no need to create
    return -1;
  }
  free( pfs_dirs[dir_id]->name );
  pfs_dirs[dir_id]->name = NULL;
  return 0;
}


struct dirent * pfs_readdir( pfs_dir_t * dir )
{
  return NULL;
}


void pfs_closedir( pfs_dir_t * dir )
{
  return;
}


void pfs_rewinddir( pfs_dir_t * dir )
{
  // makes no sense (yet?) to support that
  return;
}




char *pfs_flags_conv(int m)
{
  memset( pfs_flag, 0, 3 );
  if (m == O_APPEND) {ESP_LOGV(TAG, "O_APPEND"); return "a+";}
  if (m == O_RDONLY) {ESP_LOGV(TAG, "O_RDONLY"); return "r";}
  if (m & O_WRONLY)  {ESP_LOGV(TAG, "O_WRONLY"); return "w";}
  if (m & O_RDWR)    {ESP_LOGV(TAG, "O_RDWR");   return "r+";}
  if (m & O_EXCL)    {ESP_LOGV(TAG, "O_EXCL");   return "w";}
  if (m & O_CREAT)   {ESP_LOGV(TAG, "O_CREAT");  return "w";}
  if (m & O_TRUNC)   {ESP_LOGV(TAG, "O_TRUNC");  return "w";}
  return pfs_flag;
}


static int vfs_pfs_fopen( const char * path, int flags, int mode )
{
  pfs_file_t* tmp = pfs_fopen( path, pfs_flags_conv(mode) );
  if( tmp != NULL ) {
    return tmp->file_id;
  }
  return -1;
}


static ssize_t vfs_pfs_read( int fd, void * dst, size_t size)
{
  if( pfs_files[fd] == NULL ) return 0;
  return pfs_fread( dst, size, 1, pfs_files[fd] );
}


static ssize_t vfs_pfs_write( int fd, const void * data, size_t size)
{
  if( pfs_files[fd] == NULL ) return 0;
  return pfs_fwrite( data, size, 1, pfs_files[fd] );
}


static int vfs_pfs_close(int fd)
{
  if( pfs_files[fd] == NULL ) return -1;
  pfs_fclose( pfs_files[fd] );
  return fd;
}

static int vfs_pfs_fsync(int fd)
{
  // not sure it's needed with ramdisk
  return fd;
}

static int vfs_pfs_fstat( int fd, struct stat * st)
{
  if( pfs_files[fd] == NULL ) return -1;
  int res = pfs_stat( pfs_files[fd]->name, st );
  if( res == 1 ) return -1;
  return 0;
}

static int vfs_pfs_stat( const char * path, struct stat * st)
{
  int res = pfs_stat( path, st );
  if( res == 1 ) return -1;
  return 0;
}

static off_t vfs_pfs_lseek(int fd, off_t offset, int mode)
{

  if( pfs_files[fd] == NULL ) return -1;
  int res = pfs_fseek( pfs_files[fd], offset, mode );
  if( res == 0 ) return -1;
  return 0;
}

static int vfs_pfs_unlink(const char *path)
{
  int res = pfs_unlink( path ); // 0 = success, 1 = fail
  if( res == 1 ) return -1;
  return 0;
}


static int vfs_pfs_rename( const char *src, const char *dst)
{
  return pfs_rename( src, dst );
}


static int vfs_pfs_rmdir(const char* name)
{
  return pfs_rmdir( name );
}


static int vfs_pfs_mkdir(const char* name, mode_t mode)
{
  int res = pfs_mkdir( name );
  if( res == 1 ) return -1;
  return 0;
}


static DIR* vfs_pfs_opendir(const char* name)
{
  pfs_dir_t* tmp = pfs_opendir( name );
  if( tmp == NULL ) return NULL;
  return (DIR*) tmp;
}

static struct dirent* vfs_pfs_readdir(DIR* pdir)
{
  if( pfs_dirs[pdir->dd_vfs_idx] == NULL ) return NULL;
  struct dirent* tmp = pfs_readdir( pfs_dirs[pdir->dd_vfs_idx] );
  return tmp;
}

static int vfs_pfs_closedir(DIR* pdir)
{
  assert(pdir);
  if( pfs_dirs[pdir->dd_vfs_idx] == NULL ) return -1;
  pfs_closedir( pfs_dirs[pdir->dd_vfs_idx] );
  return 0;
}





esp_err_t esp_vfs_pfs_register(const esp_vfs_pfs_conf_t* conf) {

  assert(conf->base_path);

  esp_vfs_t vfs = {
    .flags       = ESP_VFS_FLAG_DEFAULT,
    .open        = &vfs_pfs_fopen,
    .read        = &vfs_pfs_read,
    .write       = &vfs_pfs_write,
    .close       = &vfs_pfs_close,
    .fsync       = &vfs_pfs_fsync,
    .fstat       = &vfs_pfs_fstat,
    .stat        = &vfs_pfs_stat,
    .lseek       = &vfs_pfs_lseek,
    .unlink      = &vfs_pfs_unlink,
    .rename      = &vfs_pfs_rename,
    .mkdir       = &vfs_pfs_mkdir,
    .rmdir       = &vfs_pfs_rmdir,    // TODO: implement this
    .opendir     = &vfs_pfs_opendir,  // TODO: implement this
    .readdir     = &vfs_pfs_readdir,  // TODO: implement this
    .closedir    = &vfs_pfs_closedir, // TODO: implement this
  };

  esp_err_t err = esp_vfs_register(conf->base_path, &vfs, NULL);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register PSramFS to \"%s\"", conf->base_path);
    return err;
  }

  ESP_LOGD(TAG, "Successfully registered PSramFS to \"%s\"", conf->base_path);

  return ESP_OK;
}


