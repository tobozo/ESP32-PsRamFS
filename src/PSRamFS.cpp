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
#include "vfs_api.h"
extern "C" {
  #include "pfs.h"
}


using namespace fs;

F_PSRam::F_PSRam(FSImplPtr impl)
    : FS(impl)
{}


bool F_PSRam::begin(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles, const char * partitionLabel)
{

  if( pfs_get_files() != NULL ) {
    log_w("Filesystem already mounted");
    return true;
  }

  if (!psramInit() ){
    log_w("No psram found, using heap");
    pfs_set_psram( false );
    pfs_set_partition_size( 100*1024 /*0.25 * ESP.getFreeHeap()*/ );
  } else {
    //pfs_set_psram( true );
    pfs_set_partition_size( FPSRAM_PARTITION_SIZE * ESP.getFreePsram() );
  }

  esp_vfs_pfs_conf_t conf = {
    .base_path = basePath,
    .partition_label = partitionLabel, // ignored ?
    .format_if_mount_failed = false
  };

  esp_err_t err = esp_vfs_pfs_register(&conf);
  if(err == ESP_FAIL && formatOnFail){
    if(format()){
      err = esp_vfs_pfs_register(&conf);
    }
  }

  if(err != ESP_OK){
    log_e("Mounting PSRAMFS failed! Error: %d", err);
    return false;
  }

  if( totalBytes() == 0 ) {
    log_e("Not enough memory free");
    return false;
  }

  _impl->mountpoint(basePath);
  log_d("Successfully set partition size at %d bytes for mount point %s", totalBytes(), basePath );
  return true;
}



void F_PSRam::end()
{
  pfs_deinit();
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
  return pfs_get_partition_size();
}


void ** F_PSRam::getFiles()
{
  return (void**)pfs_get_files();
}


void ** F_PSRam::getFolders()
{
  return (void**)pfs_get_dirs();
}


size_t F_PSRam::getFilesCount()
{
  return pfs_get_max_items();
}


size_t F_PSRam::usedBytes()
{
  return pfs_used_bytes();
}



size_t F_PSRam::freeBytes()
{
  return totalBytes() - usedBytes();
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


F_PSRam PSRamFS = F_PSRam(FSImplPtr(new VFSImpl()));
