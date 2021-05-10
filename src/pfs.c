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

#include "sdkconfig.h"

// for SPIRAM detection support
#ifdef CONFIG_IDF_CMAKE // IDF 4+
  #if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
    #include "esp32/spiram.h"
  #elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/spiram.h"
    #include "esp32s2/rom/cache.h"
  #else
    #error Target CONFIG_IDF_TARGET is not supported
  #endif
#else // ESP32 Before IDF 4.0
  #include "esp_spiram.h"
#endif

// for later support
#if __has_include("esp_arduino_version.h")
  #include "esp_arduino_version.h"
#endif

#if __has_include("esp_idf_version.h")
  #include "esp_idf_version.h"
#endif


#include "pfs.h"
#include "esp_vfs.h"


// for debug
static const char TAG[] = "esp_psramfs";
// this is more of a preference, lack of detection will have psram disabled
bool pfs_psram_enabled = true;
// these are the defaults in the optimal scenario (psram detected), otherwise overwritten
size_t pfs_alloc_block_size = 4096;
int pfs_max_items  = 256;

size_t pfs_partition_size = 0;
char* pfs_partition_label;

// files and directories holders, an array with [pfs_max_items] items
// initialized when vfs is registered
pfs_file_t ** pfs_files;
pfs_dir_t  ** pfs_dirs;

// choosing the alloc system (should defaut to psram but who knows)

// using psram
void*    p_malloc(size_t size) { return heap_caps_malloc( size, MALLOC_CAP_SPIRAM ); }
void*    p_calloc(size_t n, size_t size) { return heap_caps_calloc( n, size, MALLOC_CAP_SPIRAM ); }
void*    p_realloc(void *ptr, size_t size)  { return heap_caps_realloc( ptr, size, MALLOC_CAP_SPIRAM ); }
uint32_t p_free() { return heap_caps_get_free_size(MALLOC_CAP_SPIRAM); }
// using iram
void*    i_malloc(size_t size) { return heap_caps_malloc( size, MALLOC_CAP_8BIT ); }
void*    i_calloc(size_t n, size_t size) { return heap_caps_calloc( n, size, MALLOC_CAP_8BIT ); }
void*    i_realloc(void *ptr, size_t size)  { return heap_caps_realloc( ptr, size, MALLOC_CAP_8BIT ); }
uint32_t i_free() { return heap_caps_get_free_size(MALLOC_CAP_8BIT); }
// aliases
void*    (*pfs_malloc)(size_t size);
void*    (*pfs_calloc)(size_t n, size_t size);
void*    (*pfs_realloc)(void *ptr, size_t size);
uint32_t (*pfs_free_mem)(void);



pfs_file_t ** pfs_get_files();
pfs_dir_t  ** pfs_get_dirs();
int         pfs_get_max_items();
void        pfs_set_max_items(size_t max_items); // applies to both files and directories
size_t      pfs_get_block_size();
void        pfs_set_block_size(size_t block_size); // smaller value = more calls to realloc()
size_t      pfs_get_partition_size();
void        pfs_set_partition_size( size_t size );
bool        pfs_get_psram();
void        pfs_set_psram( bool use );
size_t      pfs_used_bytes();
void        pfs_init( const char * partition_label );
void        pfs_init_dirs();
int         pfs_next_file_avail();
int         pfs_next_dir_avail();
int         pfs_find_file( const char* path );
int         pfs_find_dir( const char* path );
char*       pfs_flags_conv_str(int m);
int         pfs_flags_conv(int m);
int         pfs_stat( const char * path, struct stat * stat_ );
pfs_file_t* pfs_fopen( const char * path, int flags, int mode );
size_t      pfs_fread( uint8_t *buf, size_t size, size_t count, pfs_file_t * stream );
size_t      pfs_fwrite( const uint8_t *buf, size_t size, size_t count, pfs_file_t * stream);
int         pfs_fflush(pfs_file_t * stream);
int         pfs_fseek( pfs_file_t * stream, off_t offset, pfs_seek_mode mode );
size_t      pfs_ftell( pfs_file_t * stream );
void        pfs_fclose( pfs_file_t * stream );
int         pfs_unlink( const char * path );
void        pfs_clean_files();
int         pfs_rename( const char * from, const char * to );
pfs_dir_t*  pfs_opendir( const char * path );
int         pfs_mkdir( const char* path );
int         pfs_rmdir( const char* path );
struct dirent* pfs_readdir( pfs_dir_t * dir );
void        pfs_closedir( pfs_dir_t * dir );
void        pfs_rewinddir( pfs_dir_t * dir );
void        pfs_free();




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

esp_err_t esp_vfs_pfs_register(const esp_vfs_pfs_conf_t* conf);
esp_err_t esp_vfs_pfs_format(const char* partition_label);
esp_err_t esp_vfs_pfs_unregister(const char* base_path);



pfs_file_t ** pfs_get_files()
{
  return pfs_files;
}


pfs_dir_t  ** pfs_get_dirs()
{
  return pfs_dirs;
}


int pfs_get_max_items()
{
  return pfs_max_items;
}


void pfs_set_max_items(size_t max_items)
{
  ESP_LOGD(TAG, "Setting max items to %d", pfs_max_items);
  pfs_max_items = max_items;
}


size_t pfs_get_block_size()
{
  return pfs_alloc_block_size;
}


void pfs_set_block_size(size_t block_size)
{
  ESP_LOGD(TAG, "Setting alloc block size to %d", block_size);
  pfs_alloc_block_size = block_size;
}


void pfs_set_alloc_functions()
{
  #ifdef BOARD_HAS_PSRAM
    if (esp_spiram_init() != ESP_OK) {
      ESP_LOGE(TAG, "Can't init psram :-(");
      pfs_psram_enabled = false;
    }
    if( pfs_psram_enabled ) {
      ESP_LOGD(TAG, "pfs will use psram");
      pfs_malloc    = p_malloc;
      pfs_realloc   = p_realloc;
      pfs_calloc    = p_calloc;
      pfs_free_mem  = p_free;
      pfs_set_max_items( 256 );
      pfs_set_block_size( 4096 );
    } else {
      ESP_LOGD(TAG, "pfs will use heap by config");
      pfs_malloc    = i_malloc;
      pfs_realloc   = i_realloc;
      pfs_calloc    = i_calloc;
      pfs_free_mem  = i_free;
      pfs_set_max_items( 16 );
      pfs_set_block_size( 512 );
    }
  #else // ! BOARD_HAS_PSRAM
    ESP_LOGD(TAG, "pfs will use heap");
    pfs_psram_enabled = false;
    pfs_malloc    = i_malloc;
    pfs_realloc   = i_realloc;
    pfs_calloc    = i_calloc;
    pfs_free_mem  = i_free;
    pfs_set_max_items( 16 );
    pfs_set_block_size( 512 );
  #endif
}


void pfs_set_psram( bool use )
{
  ESP_LOGD(TAG, "%s psram...", use ? "Enabling" : "Disabling");
  pfs_psram_enabled = use;
}


bool pfs_get_psram()
{
  return pfs_psram_enabled;
}


void pfs_set_partition_size( size_t size )
{
  pfs_partition_size = size;
}


size_t pfs_get_partition_size()
{
  return pfs_partition_size;
}


void pfs_init( const char* partition_label )
{
  pfs_set_psram( pfs_psram_enabled );
  pfs_set_alloc_functions();

  pfs_files = (pfs_file_t**)pfs_calloc( pfs_max_items, sizeof( pfs_file_t* ) );
  if( pfs_files == NULL ) {
    ESP_LOGE(TAG, "Unable to init pfs, halting");
    while(1);
  }
  for( int i=0; i<pfs_max_items; i++ ) {
    pfs_files[i] = (pfs_file_t*)pfs_calloc( 1, sizeof( pfs_file_t ) );
    if( pfs_files[i] == NULL ) {
      ESP_LOGE(TAG, "Unable to init pfs, halting");
      while(1);
    }
  }
  // this may be redundant with vfs properties
  if( pfs_partition_label != NULL ) free(pfs_partition_label);
  pfs_partition_label = (char*)calloc( 1, strlen( partition_label )+1 );
  snprintf( (char*)pfs_partition_label, strlen( partition_label ), "%s", partition_label );

  pfs_init_dirs();

  ESP_LOGD(TAG, "Init files OK");
}


void pfs_init_dirs()
{
  pfs_dirs = (pfs_dir_t **)pfs_calloc( pfs_max_items, sizeof( pfs_dir_t* ) );
  if( pfs_dirs == NULL ) {
    ESP_LOGE(TAG, "Unable to init pfs, halting");
    while(1);
  }
  for( int i=0; i<pfs_max_items; i++ ) {
    pfs_dirs[i] = (pfs_dir_t*)pfs_calloc( 1, sizeof( pfs_dir_t ) );
    if( pfs_dirs[i] == NULL ) {
      ESP_LOGE(TAG, "Unable to init pfs, halting");
      while(1);
    }
  }
  ESP_LOGD(TAG, "Init dirs OK");
}


int pfs_next_file_avail()
{
  int res = 0;
  if( pfs_files != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_files[i]->name == NULL ) {
        ESP_LOGV(TAG, "File Slot %d out of %d is free [r]", i, pfs_max_items );
        return i;
      }
    }
    ESP_LOGE(TAG, "Too many files created.");
  } else {
    ESP_LOGE(TAG, "No allocated space for files");
  }
  return res;
}


int pfs_next_dir_avail()
{
  int res = 0;
  if( pfs_dirs != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_dirs[i]->name == NULL ) {
        ESP_LOGV(TAG, "Dir Slot %d out of %d is free [r]", i, pfs_max_items );
        return i;
      }
    }
    ESP_LOGE(TAG, "Too many dirs created, halting");
    while(1);
  } else {
    ESP_LOGE(TAG, "No allocated space for dirs");
  }
  return res;
}


int pfs_find_file( const char* path )
{
  if( pfs_files != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_files[i]->name == NULL ) continue;
      if( strcmp( path, pfs_files[i]->name ) == 0 ) {
        return i;
      }
    }
  }
  return -1;
}


int pfs_find_dir( const char* path )
{
  if( pfs_dirs != NULL ) {
    unsigned long entitycount = 0;
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_dirs[i]->name == NULL ) continue;
      if( strcmp( path, pfs_dirs[i]->name ) == 0 ) {
        return i;
      }
    }
  } else {
    ESP_LOGW(TAG, "Call on pfs_find_dir() before pfs_dirs are allocated");
  }
  return -1;
}


size_t pfs_used_bytes()
{
  size_t totalsize = 0;
  if( pfs_files != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_files[i]->name != NULL ) {
        //totalsize += pfs_files[i]->size;
        totalsize += pfs_files[i]->memsize;
        //ESP_LOGV(TAG, "Adding %d bytes from %s (total=%d)", pfs_files[i]->size, pfs_files[i]->name, totalsize );
      }
    }
  } else {
    ESP_LOGW(TAG, "Call on used_bytes() before pfs_files are allocated");
  }
  return totalsize;
}


int pfs_stat( const char * path, struct stat * stat_ )
{
  assert(path);

  memset(stat_, 0, sizeof(* stat_));

  if(path == NULL) {
    ESP_LOGW(TAG, "NOT stating for null path");
    return 1;
  }

  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    stat_->st_size    = pfs_files[file_id]->size;
    stat_->st_mode    = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
    stat_->st_mode    = S_IFREG;
    ESP_LOGD(TAG, "stating for DT_REG(%s) success (size=%d)", path, pfs_files[file_id]->size );
    return 0;
  } else {
    ESP_LOGD(TAG, "stating for DT_REG(%s) no pass", path );
  }

  int dir_id = pfs_find_dir( path );
  if( dir_id > -1 ) {
    stat_->st_size = 0;
    stat_->st_mode    = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
    stat_->st_mode    = S_IFDIR;
    ESP_LOGD(TAG, "stating for DT_DIR(%s) success", path );
    return 0;
  } else {
    ESP_LOGD(TAG, "stating for DT_DIR(%s) no pass", path );
  }

  stat_->st_mode = DT_UNKNOWN;

  return 1;
}




char *pfs_flags_conv_str(int m)
{
  if( (m & O_WRONLY) && (m & O_TRUNC) ) return "w";
  if( (m & O_RDWR ) && (m & O_TRUNC) ) return "w+";
  if( m & O_RDWR ) return "r+";
  if( m & O_WRONLY ) return "a";
  if( m & O_RDWR ) return "a+";
  if( m & O_RDONLY ) return "r";
  return "r";
}


int pfs_flags_conv(int m)
{
  int pfs_flags = 0;
  if (m == O_APPEND) {ESP_LOGV(TAG, "O_APPEND"); pfs_flags |= PFS_O_APPEND;}
  if (m == O_RDONLY) {ESP_LOGV(TAG, "O_RDONLY"); pfs_flags |= PFS_O_RDONLY;}
  if (m & O_WRONLY)  {ESP_LOGV(TAG, "O_WRONLY"); pfs_flags |= PFS_O_WRONLY;}
  if (m & O_RDWR)    {ESP_LOGV(TAG, "O_RDWR");   pfs_flags |= PFS_O_RDWR;}
  if (m & O_EXCL)    {ESP_LOGV(TAG, "O_EXCL");   pfs_flags |= PFS_O_EXCL;}
  if (m & O_CREAT)   {ESP_LOGV(TAG, "O_CREAT");  pfs_flags |= PFS_O_CREAT;}
  if (m & O_TRUNC)   {ESP_LOGV(TAG, "O_TRUNC");  pfs_flags |= PFS_O_TRUNC;}
  return pfs_flags;
}



pfs_file_t* pfs_fopen( const char * path, int flags, int fmode )
{

  if( path == NULL ) {
    ESP_LOGE(TAG, "Invalid path");
    return NULL;
  } else {
    ESP_LOGV(TAG, "Valid path :%s, flags = 0x%04x, mode = 0x%04x", path, flags, fmode );
  }

  char *mode = pfs_flags_conv_str(flags);
  int newflags = pfs_flags_conv(flags);

  int file_id = pfs_find_file( path );

  if( file_id > -1 ) {

    ESP_LOGV(TAG, "Old flags: 0x%08x, New flags: 0x%08x", pfs_files[file_id]->flags, newflags );
    pfs_files[file_id]->flags = newflags;

    // existing file
    if( mode ) {
      switch( mode[0] ) {
        case 'a': // seek end
          ESP_LOGV(TAG, "Append to index :%d", pfs_files[file_id]->size );
          fseek( (FILE*)pfs_files[file_id], 0, pfs_seek_end );
          pfs_files[file_id]->index = ftell( (FILE*)pfs_files[file_id] );
        break;
        case 'w': // truncate
          ESP_LOGV(TAG, "Truncate (mode=%s)", mode);
          if( pfs_files[file_id]->bytes != NULL ) {
            free( pfs_files[file_id]->bytes );
            pfs_files[file_id]->bytes = NULL;
          }
          pfs_files[file_id]->index = 0;
          pfs_files[file_id]->size  = 0;
        break;
        case 'r':
          ESP_LOGV(TAG, "Read (mode=%s)", mode);
          pfs_files[file_id]->index = 0;
        break;
        default:
          ESP_LOGV(TAG, "Unchanged index (mode=%s)", mode);
        break;
      }
    }
    ESP_LOGD(TAG, "file exists: %s (mode %s)", path, mode);
    return pfs_files[file_id];
  }

  if(mode && (mode[0] != 'r' || mode[1] == '+') ) {
    // new file, write mode
    int slotscount = 0;
    int fileslot = pfs_next_file_avail();

    if( fileslot < 0 || pfs_files[fileslot] == NULL ) {
      ESP_LOGE(TAG, "alloc fail!");
      return NULL;
    }
    if( pfs_files[fileslot]->name != NULL ) { // uh-oh this should not happen
      ESP_LOGE(TAG, "Name from file slot #%d is now null, freeing", fileslot);
      free( pfs_files[fileslot]->name );
    }
    int pathlen = strlen(path);
    pfs_files[fileslot]->name = (char*)pfs_malloc( pathlen+1 );
    memcpy( pfs_files[fileslot]->name, path, pathlen+1 );
    pfs_files[fileslot]->index = 0; // default truncate
    pfs_files[fileslot]->size  = 0;
    pfs_files[fileslot]->file_id = fileslot;
    pfs_files[fileslot]->flags = newflags;
    ESP_LOGD(TAG, "file created: %s (mode: %s, flags: 0x%08x)", path, mode, newflags);
    return pfs_files[fileslot];
  }
  ESP_LOGE(TAG, "can't open: %s (mode %s)", path, mode);
  return NULL;
}





size_t pfs_fread( uint8_t *buf, size_t size, size_t count, pfs_file_t * stream )
{
  size_t to_read = size*count;

  if( ( stream->index + to_read ) >= stream->size ) {
    if( stream->index <= stream->size ) {
      to_read = stream->size - stream->index;
      if( to_read == 0 ) return 0;
    } else {
      ESP_LOGE(TAG, "Attempted to read %d out of bounds bytes at index %d of %d", to_read, stream->index, stream->size );
      return -1;
    }
  }
  memcpy( buf, &stream->bytes[stream->index], to_read );
  if( to_read > 1 ) {
    ESP_LOGV(TAG, "Reading %d byte(s) at index %d of %d", to_read, stream->index, stream->size );
  } else {
    char out[2] = {0,0};
    out[0] = buf[0];
    ESP_LOGV(TAG, "Reading %d byte(s) at index %d of %d (%s)", to_read, stream->index, stream->size, out );
  }
  stream->index += to_read;
  return to_read;
}



size_t pfs_fwrite( const uint8_t *buf, size_t size, size_t count, pfs_file_t * stream)
{
  size_t to_write = size*count;
  if( stream->index + to_write >= stream->memsize ) {

    size_t used_bytes = pfs_used_bytes();

    if( stream->bytes == NULL ) {
      if( pfs_partition_size > 0 && used_bytes + pfs_alloc_block_size > pfs_partition_size ) {
        ESP_LOGE(TAG, "Not enough memory left, cowardly aborting (partition size=%d, used_bytes=%d, needs %d bytes)", pfs_partition_size, used_bytes, used_bytes + pfs_alloc_block_size );
        return -1;
      }
      ESP_LOGV(TAG, "Allocating %d bytes to write %d bytes", pfs_alloc_block_size, to_write );
      stream->bytes = (char*)pfs_calloc( 1, pfs_alloc_block_size /*, sizeof(char) */);
      stream->memsize = pfs_alloc_block_size;
      used_bytes += pfs_alloc_block_size;
    }
    while( stream->index + to_write >= stream->memsize ) {
      if( pfs_partition_size > 0 && used_bytes + pfs_alloc_block_size > pfs_partition_size ) {
        ESP_LOGE(TAG, "Not enough memory left, cowardly aborting (partition size=%d, used_bytes=%d wants %d bytes)", pfs_partition_size, used_bytes, used_bytes + pfs_alloc_block_size );
        return -1;
      }
      ESP_LOGV(TAG, "[bytes free:%d] Reallocating %d bytes to write %d bytes at index %d/%d => %d", pfs_free_mem(), pfs_alloc_block_size, to_write, stream->index, stream->size, stream->index + pfs_alloc_block_size  );
      ESP_LOGV(TAG, "stream->bytes = (char*)realloc( %d, %d ); (when %d/%d bytes free)", stream->bytes, stream->index + pfs_alloc_block_size, pfs_free_mem(), pfs_partition_size  );
      stream->bytes = (char*)pfs_realloc( stream->bytes, stream->memsize + pfs_alloc_block_size  );
      stream->memsize += pfs_alloc_block_size;
      used_bytes += pfs_alloc_block_size;
    }
  } else {
    ESP_LOGV(TAG, "Writing %d bytes at index %d of %d (no realloc, memsize = %d)", to_write, stream->index, stream->size, stream->memsize );
  }
  // There should be a check here to make sure this write does not exceed the file maximum size
  memcpy( &stream->bytes[stream->index], buf, to_write );
  stream->index += to_write;
  if (stream->index > stream->size) stream->size  = stream->index;

  return to_write;
}



int pfs_fflush(pfs_file_t * stream)
{
  ESP_LOGW("[FIXME] Flushing (actually does nothing)");
  return 0;
}



int pfs_fseek( pfs_file_t * stream, off_t offset, pfs_seek_mode mode )
{
  if( offset < 0 ) offset = 0; // dafuq ?

  switch( mode ) {
    case pfs_seek_set: // 0
      if( offset < stream->size ) {
        stream->index = offset;
        ESP_LOGV(TAG, "Seeking mode #%d (seekset) with offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
      } else {
        stream->index = stream->size;
        ESP_LOGE(TAG, "Seeking mode #%d (seekset) with capped offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
        return -1;
      }
    break;
    case pfs_seek_cur: // 1
      if( stream->index + offset <= stream->size ) {
        stream->index += offset;
        ESP_LOGV(TAG, "Seeking mode #%d (seekcur) with offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
      } else {
        stream->index = stream->size;
        ESP_LOGE(TAG, "Seeking mode #%d (seekcur) with truncated offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
        return -1;
      }
    break;
    case pfs_seek_end: // 2
      if( offset == 0 ) {
        stream->index = stream->size;
      } else {
        if( offset <= stream->size ) {
          stream->index = stream->size - offset;
        } else {
          stream->index = 0;
          ESP_LOGE(TAG, "Seeking mode #%d (seekend) with truncated offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
          return -1;
        }
      }
      ESP_LOGV(TAG, "Seeking mode #%d (seekend) with offset(%d)/size(%d)/index(%d)", mode, offset, stream->size, stream->index );
    break;
  }
  return 0;
}



size_t pfs_ftell( pfs_file_t * stream )
{
  if( stream == NULL ) {
    ESP_LOGE(TAG, "Invalid stream");
    return -1;
  }
  ESP_LOGD(TAG, "Ftelling index %d", stream->index);
  return stream->index;
}



void pfs_fclose( pfs_file_t * stream )
{
  if( stream->size < 128 ) {
    ESP_LOGD(TAG, "Closing stream %s (%d bytes) contents: %s\n", stream->name, stream->size, (const char*)stream->bytes);
  } else {
    ESP_LOGD(TAG, "Closing stream %s (%d bytes)\n", stream->name, stream->size );
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
    pfs_files[file_id]->file_id = -1;
    ESP_LOGD(TAG, "Path %s unlinked successfully", path);
    return 0;
  }
  ESP_LOGW(TAG, "Path %s not found, can't unlink", path);
  return 1;
}


void pfs_free()
{
  if( pfs_files != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_files[i]->name != NULL ) {
        pfs_unlink( pfs_files[i]->name );
        free( pfs_files[i] );
      }
    }
  }
  if( pfs_dirs != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_dirs[i]->name != NULL ) {
        //pfs_unlink( pfs_files[i]->name );
        free( pfs_dirs[i]->name );
        pfs_dirs[i]->name = NULL;
        free( pfs_dirs[i] );
      }
    }
  }
  if( pfs_files != NULL ) {
    free( pfs_files );
    pfs_files = NULL;
  }
  if( pfs_dirs  != NULL ) {
    free( pfs_dirs );
    pfs_dirs = NULL;
  }
  if( pfs_partition_label != NULL ) {
    free( pfs_partition_label );
    pfs_partition_label = NULL;
  }
}


void pfs_clean_files()
{
  if( pfs_files != NULL ) {
    for( int i=0; i<pfs_max_items; i++ ) {
      if( pfs_files[i]->name != NULL ) {
        pfs_unlink( pfs_files[i]->name );
      }
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
    pfs_files[file_id]->name = (char*)pfs_malloc( strlen(to)+1 );
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
    pfs_dirs[dir_id]->name = (char*)pfs_malloc( strlen(to)+1 );
    memcpy( pfs_dirs[dir_id]->name, to, strlen(to)+1 );
    return 0;
  }

  return -1;
}


pfs_dir_t* pfs_opendir( const char * path )
{

  int file_id = pfs_find_file( path );
  if( file_id > -1 ) {
    ESP_LOGW(TAG, "Can't create dir(%s) as a file already exists with the same name", path );
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
  pfs_dirs[dirslot]->name = (char*)pfs_calloc(1, sizeof( path ) +1 );

  if( pfs_dirs[dirslot] == NULL || pfs_dirs[dirslot]->name == NULL ) {
    ESP_LOGE(TAG, "Failed to create dir %s", path );
    return 1;
  }

  pfs_dirs[dirslot]->dir_id = dirslot;
  memcpy( pfs_dirs[dirslot]->name, path, sizeof( path ) +1 );

  ESP_LOGD(TAG, "Created dir %s", path );
  return 0;

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
  ESP_LOGD(TAG, "[FIXME] Fake readdir (does nothing)");
  return NULL;
}


void pfs_closedir( pfs_dir_t * dir )
{
  ESP_LOGD(TAG, "[FIXME] Fake closedir (does nothing)");
  return;
}


void pfs_rewinddir( pfs_dir_t * dir )
{
  // makes no sense (yet?) to support that
  ESP_LOGD(TAG, "[FIXME] Fake rewinddir (does nothing)");
  return;
}


int vfs_pfs_fopen( const char * path, int flags, int mode )
{

  pfs_file_t* tmp = pfs_fopen( path, flags, mode );
  if( tmp != NULL ) {
    return tmp->file_id;
  }
  return -1;
}


ssize_t vfs_pfs_read( int fd, void * dst, size_t size)
{
  if( pfs_files[fd] == NULL ) return 0;
  return pfs_fread( dst, size, 1, pfs_files[fd] );
}


ssize_t vfs_pfs_write( int fd, const void * data, size_t size)
{
  if( pfs_files[fd] == NULL ) return 0;
  return pfs_fwrite( data, size, 1, pfs_files[fd] );
}


int vfs_pfs_close(int fd)
{
  if( pfs_files[fd] == NULL ) return -1;
  pfs_fclose( pfs_files[fd] );
  return 0;
}

int vfs_pfs_fsync(int fd)
{
  // not sure it's needed with ramdisk
  return fd;
}

int vfs_pfs_fstat( int fd, struct stat * st)
{
  assert(st);
  if( pfs_files[fd] == NULL ) {
    ESP_LOGE(TAG, "Invalid file descriptor (%d)", fd);
    return -1;
  }
  struct stat s;
  int res = pfs_stat( pfs_files[fd]->name, &s );
  if( res == 1 ) return -1;
  memset(st, 0, sizeof(*st));
  st->st_size = s.st_size;
  st->st_mode = s.st_mode;//S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
  return res;
}

int vfs_pfs_stat( const char * path, struct stat * st)
{
  int res = pfs_stat( path, st );
  if( res == 1 ) return -1;
  return 0;
}

off_t vfs_pfs_lseek(int fd, off_t offset, int mode)
{

  if( pfs_files[fd] == NULL ) return -1;
  if ( pfs_fseek( pfs_files[fd], offset, mode ) == 0 )
     return offset;
  return -1;
}

int vfs_pfs_unlink(const char *path)
{
  int res = pfs_unlink( path ); // 0 = success, 1 = fail
  if( res == 1 ) return -1;
  return 0;
}


int vfs_pfs_rename( const char *src, const char *dst)
{
  return pfs_rename( src, dst );
}


int vfs_pfs_rmdir(const char* name)
{
  return pfs_rmdir( name );
}


int vfs_pfs_mkdir(const char* name, mode_t mode)
{
  int res = pfs_mkdir( name );
  if( res == 1 ) return -1;
  return 0;
}


DIR* vfs_pfs_opendir(const char* name)
{
  pfs_dir_t* tmp = pfs_opendir( name );
  if( tmp == NULL ) return NULL;
  return (DIR*) tmp;
}

struct dirent* vfs_pfs_readdir(DIR* pdir)
{
  if( pfs_dirs[pdir->dd_vfs_idx] == NULL ) return NULL;
  struct dirent* tmp = pfs_readdir( pfs_dirs[pdir->dd_vfs_idx] );
  return tmp;
}

int vfs_pfs_closedir(DIR* pdir)
{
  assert(pdir);
  if( pfs_dirs[pdir->dd_vfs_idx] == NULL ) return -1;
  pfs_closedir( pfs_dirs[pdir->dd_vfs_idx] );
  return 0;
}


size_t vfs_pfs_ftell(FILE* stream)
{
  return pfs_ftell( (pfs_file_t*)stream );
}



esp_err_t esp_vfs_pfs_register(const esp_vfs_pfs_conf_t* conf)
{
  assert(conf->base_path);
  assert(conf->partition_label);

  esp_vfs_t vfs_pfs = {
    .flags       = ESP_VFS_FLAG_DEFAULT,
    .open        = &vfs_pfs_fopen,
    .read        = &vfs_pfs_read,
    .write       = &vfs_pfs_write,
    .close       = &vfs_pfs_close,
    .fsync       = &vfs_pfs_fsync,
    //.ftell       = &vfs_pfs_ftell, // you wish
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

  esp_err_t err = esp_vfs_register(conf->base_path, &vfs_pfs, NULL);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register PSramFS to \"%s\"", conf->base_path);
    return err;
  }

  pfs_init( conf->partition_label );

  ESP_LOGD(TAG, "Successfully registered PSramFS to \"%s\"", conf->base_path);

  return ESP_OK;
}



esp_err_t esp_vfs_pfs_format(const char* base_path)
{
  ESP_LOGD(TAG, "Formatting \"%s\"",  base_path );

  pfs_clean_files();

  return ESP_OK;
}



esp_err_t esp_vfs_pfs_unregister(const char* base_path)
{
  assert(base_path);

  ESP_LOGD(TAG, "Unregistering \"%s\"",  base_path );

  esp_err_t err = esp_vfs_unregister( base_path );

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to unregister \"%s\" (err=%d)", base_path, err);
    return err;
  }

  pfs_free();

  return ESP_OK;
}



esp_err_t esp_vfs_pfs_info(const char* partition_label, size_t *total_bytes, size_t *used_bytes)
{
  // this makes no sense as there is no "real" partition
  if( pfs_partition_label == NULL || pfs_partition_label[0] == '\0' ) {
    return ESP_ERR_INVALID_STATE;
  }

  *total_bytes = pfs_used_bytes();
  *used_bytes  = pfs_get_partition_size();
  return ESP_OK;
}
