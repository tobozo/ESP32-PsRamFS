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


struct PSRAMFILE
{
  char * name;
  char * bytes;
  unsigned long size;
  unsigned long memsize;
  unsigned long index;
};

struct PSRAMDIR
{
  char * name = NULL;
};

struct pfs_stat_t
{
  uint8_t  st_mode;
  uint32_t st_mtime;
  size_t   st_size;
};

struct dirent {
    int d_ino;          /*!< file number */
    uint8_t d_type;     /*!< not defined in POSIX, but present in BSD and Linux */
#define DT_UNKNOWN  0
#define DT_REG      1
#define DT_DIR      2
    char d_name[256];   /*!< zero-terminated file name */
};


PSRAMFILE ** pfs_get_files();
PSRAMDIR  ** pfs_get_dirs();

int pfs_max_items();
void pfs_free();
void pfs_clean_files();
bool pfs_stat( const char * path, const void *_stat );
PSRAMFILE* pfs_fopen( const char * path, const char* mode );
size_t pfs_fread( uint8_t *buf, size_t size, size_t count, PSRAMFILE * stream );
size_t pfs_fwrite( const uint8_t *buf, size_t size, size_t count, PSRAMFILE * stream);
int pfs_fflush(PSRAMFILE * stream);
int pfs_fseek( PSRAMFILE * stream, long offset, SeekMode mode );
size_t pfs_ftell( PSRAMFILE * stream );
void pfs_fclose( PSRAMFILE * stream );
int pfs_unlink( char * path );
int pfs_rename( char * from, char * to );
PSRAMDIR* pfs_opendir( char * path );
int pfs_mkdir( char* path );
struct dirent * pfs_readdir( PSRAMDIR * dir );
void pfs_closedir( PSRAMDIR * dir );
void pfs_rewinddir( PSRAMDIR * dir );

#ifdef __cplusplus
}
#endif

#endif
