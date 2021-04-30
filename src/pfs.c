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

#include <string.h>

// from dirent.h
#define DT_UNKNOWN  0
#define DT_REG      1
#define DT_DIR      2

#include "esp_heap_caps.h"
#include "esp32-hal-log.h"
#define realloc(p,s)    heap_caps_realloc(p, s, MALLOC_CAP_8BIT)
#define ps_malloc(p)    heap_caps_malloc(p, MALLOC_CAP_SPIRAM)
#define ps_realloc(p,s) heap_caps_realloc(p, s, MALLOC_CAP_SPIRAM)
#define ps_calloc(p, s) heap_caps_calloc(p, s, MALLOC_CAP_SPIRAM)
#define ALLOC_BLOCK_SIZE 4094
#define MAX_PSRAM_FILES 256

typedef enum {
  SeekSet = 0,
  SeekCur = 1,
  SeekEnd = 2
} SeekMode;


typedef struct
{
  char * name;
  char * bytes;
  unsigned long size;
  unsigned long memsize;
  unsigned long index;
} PSRAMFILE;


typedef struct
{
  char * name;
} PSRAMDIR;


typedef struct
{
  unsigned int st_mode;
  unsigned long st_mtime;
  unsigned long st_size;
  const char* st_name;
} psramStat_t;


PSRAMFILE ** pfs_files;
PSRAMDIR  ** pfs_dirs;


int pfs_files_set = -1;
int pfs_dirs_set = -1;


PSRAMFILE ** pfs_get_files() { return pfs_files; }
PSRAMDIR  ** pfs_get_dirs()  { return pfs_dirs;  }

int pfs_max_items()
{
  return MAX_PSRAM_FILES;
}



void pfs_init_files()
{
  pfs_files = (PSRAMFILE**)ps_calloc( MAX_PSRAM_FILES, sizeof( PSRAMFILE* ) );
  if( pfs_files == NULL ) {
    log_e("Unable to init psram fs, halting");
    while(1);
  }
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    pfs_files[i] = (PSRAMFILE*)ps_calloc( 1, sizeof( PSRAMFILE ) );
    if( pfs_files[i] == NULL ) {
      log_e("Unable to init psram fs, halting");
      while(1);
    }
  }
  log_d("Psram init files OK");
}


void pfs_init_dirs()
{
  pfs_dirs = (PSRAMDIR **)ps_calloc( MAX_PSRAM_FILES, sizeof( PSRAMDIR* ) );
  if( pfs_dirs == NULL ) {
    log_e("Unable to init psram fs, halting");
    while(1);
  }
  for( int i=0; i<MAX_PSRAM_FILES; i++ ) {
    pfs_dirs[i] = (PSRAMDIR*)ps_calloc( 1, sizeof( PSRAMDIR ) );
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
  }
  log_e("Too many files created, halting");
  while(1);
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
  psramStat_t* stat_ = (psramStat_t*)_stat;
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



PSRAMFILE* pfs_fopen( const char * path, const char* mode )
{

  if( path == NULL ) {
    log_e("Invalid path");
    return NULL;
  } else {
    log_v("valid path :%s", path );
  }
  int pathlen = strlen(path);

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

  if(mode && mode[0] != 'r') {
    // new file, write mode
    int slotscount = 0;
    int fileslot = pfs_next_file_avail();

    if( pfs_files[fileslot] == NULL ) {
      log_e("alloc fail!");
      return NULL;
    }
    if( pfs_files[fileslot]->name != NULL ) {
      log_e("Name from file slot #%d is now null, freeing", fileslot);
      free( pfs_files[fileslot]->name );
    }
    pfs_files[fileslot]->name = (char*)ps_malloc( pathlen+1 );
    memcpy( pfs_files[fileslot]->name, path, pathlen+1 );
    pfs_files[fileslot]->index = 0; // default truncate
    pfs_files[fileslot]->size  = 0;

    log_d( "file created: %s (mode %s)", path, mode);
    return pfs_files[fileslot];
  }
  log_e("can't open: %s (mode %s)", path, mode);
  return NULL;
}




size_t pfs_fread( uint8_t *buf, size_t size, size_t count, PSRAMFILE * stream )
{
  size_t to_read = size*count;
  if( ( stream->index + to_read ) > stream->size ) {
    log_e("Attempted to read %d out of bounds bytes at index %d of %d", to_read, stream->index, stream->size );
    return 1;
  }
  memcpy( buf, &stream->bytes[stream->index], to_read );
  if( to_read > 1 ) {
    log_d("Reading %d byte(s) at index %d of %d", to_read, stream->index, stream->size );
  } else {
    char out[2] = {0,0};
    out[0] = buf[0];
    log_d("Reading %d byte(s) at index %d of %d (%s)", to_read, stream->index, stream->size, out );
  }
  stream->index += to_read;
  return to_read;
}


size_t pfs_fwrite( const uint8_t *buf, size_t size, size_t count, PSRAMFILE * stream)
{
  size_t to_write = size*count;
  if( ( stream->index + to_write ) > stream->memsize ) {
    if( stream->bytes == NULL ) {
      log_d("Allocating %d bytes to write %d bytes", ALLOC_BLOCK_SIZE, to_write );
      stream->bytes = (char*)ps_malloc( ALLOC_BLOCK_SIZE /*, sizeof(char) */);
      stream->memsize = ALLOC_BLOCK_SIZE;
    } else {
      // realloc
      log_d("Reallocating %d bytes to write %d bytes at index %d/%d => %d", ALLOC_BLOCK_SIZE, to_write, stream->index, stream->size, stream->index + ALLOC_BLOCK_SIZE );
      log_d("stream->bytes = (char*)realloc( %d, %d );", stream->bytes, stream->index + ALLOC_BLOCK_SIZE );
      stream->bytes = (char*)ps_realloc( stream->bytes, stream->memsize + ALLOC_BLOCK_SIZE );
      stream->memsize += ALLOC_BLOCK_SIZE;
    }
  } else {
    log_d("Writing %d bytes at index %d of %d (no realloc, memsize = %d)", to_write, stream->index, stream->size, stream->memsize );
  }
  memcpy( &stream->bytes[stream->index], buf, to_write );
  stream->index += to_write;
  stream->size  += to_write;

  return to_write;
}

int pfs_fflush(PSRAMFILE * stream)
{
  return 0;
}


int pfs_fseek( PSRAMFILE * stream, long offset, SeekMode mode )
{
  log_v("Seeking mode #%s with offset %d", mode, offset );
  switch( mode ) {
    case SeekSet:
      if( offset < stream->size ) {
        stream->index = offset;
        break;
      }
      return -1;
    case SeekCur:
      if( stream->index + offset < stream->size ) {
        stream->index += offset;
        break;
      }
      return -1;
    case SeekEnd:
      if( (stream->size-1) - offset > stream->size ) {
        stream->index = (stream->size-1) - offset;
      }
      return -1;
      break;
  }
  return 0;
}


size_t pfs_ftell( PSRAMFILE * stream )
{
  log_d("Getting cursor for %s = %d", stream->name, stream->index+1);
  return stream->index+1;
}


void pfs_fclose( PSRAMFILE * stream )
{
  if( stream->size < 128 ) {
    log_d("Closing stream %s (%d bytes) contents: %s\n", stream->name, stream->size, (const char*)stream->bytes);
  } else {
    log_d("Closing stream %s (%d bytes)\n", stream->name, stream->size );
  }
  stream->index = 0;
  return;
}


int pfs_unlink( char * path )
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


int pfs_rename( char * from, char * to )
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

  return 1;
}



PSRAMDIR* pfs_opendir( char * path )
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


int pfs_mkdir( char* path )
{
  int dir_id = pfs_find_dir( path );
  if( dir_id > -1 ) {
    // directory exists, no need to create
    return 0;
  }

  int slotscount = 0;
  int dirslot = pfs_next_dir_avail();
  pfs_dirs[dirslot]->name = (char*)ps_calloc(1, sizeof( path ) +1 );
  memcpy( pfs_dirs[dirslot]->name, path, sizeof( path ) +1 );
  if( pfs_dirs[dirslot] != NULL ) {
    return 0;
  }
  return 1;

}


struct dirent * pfs_readdir( PSRAMDIR * dir )
{
  // makes no sense (yet?) to support that
  return NULL;
}


void pfs_closedir( PSRAMDIR * dir )
{
  // makes no sense (yet?) to support that
  return;
}


void pfs_rewinddir( PSRAMDIR * dir )
{
  // makes no sense (yet?) to support that
  return;
}




