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


#include "PSRamFS.h"
#include "PSRamFSImpl.h"
#include "pfs.h"


using namespace fs;

F_PSRam::F_PSRam(FSImplPtr impl)
    : FS(impl)
{}


bool F_PSRam::begin(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles, const char * partitionLabel)
{
  //PSRAMFILE ** pfs_files = ;

  if( pfs_get_files() != NULL ) {
    // log_v("already begun");
    return true;
  }

  if (!psramInit() ){
    log_e("No psram found");
    return false;
  }

  partitionSize = FPSRAM_PARTITION_SIZE * ESP.getFreePsram();

  if( partitionSize == 0 ) {
    log_e("Not enough psram free");
    return false;
  }

  _impl->mountpoint(basePath);
  log_d("Successfully set partition size at %d bytes for mount point %s", partitionSize, basePath );
  return true;
}



void F_PSRam::end()
{
  pfs_free();
  _impl->mountpoint(NULL);
  return;
}



bool F_PSRam::format(bool full_wipe, char* partitionLabel)
{
  pfs_clean_files();
  return true;
}



size_t F_PSRam::totalBytes()
{
  return partitionSize;
}

void ** F_PSRam::getFiles()
{
  return (void**)pfs_get_files();
}

size_t F_PSRam::usedBytes()
{
  size_t slotscount = pfs_max_items();
  size_t totalsize = 0;
  PSRAMFILE** pfs_files = pfs_get_files();
  if( pfs_files != NULL ) {
    for( int i=0; i<slotscount; i++ ) {
      if( pfs_files[i]->name != NULL ) {
        totalsize += pfs_files[i]->size;
      }
    }
  }
  return totalsize;
}



size_t F_PSRam::freeBytes()
{
  return partitionSize - usedBytes();
}



bool F_PSRam::exists(const char* path)
{
  File f = open(path, "r");
  return (f == true) && !f.isDirectory();
}



bool F_PSRam::exists(const String& path)
{
  return exists(path.c_str());
}


F_PSRam PSRamFS = F_PSRam(FSImplPtr(new PSRamFSImpl()));