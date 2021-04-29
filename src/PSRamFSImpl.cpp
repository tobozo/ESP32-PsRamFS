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

#include "PSRamFSImpl.h"


using namespace fs;

FileImplPtr PSRamFSImpl::open(const char* path, const char* mode)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return FileImplPtr();
  }

  if(!path || path[0] != '/') {
    log_e("%s does not start with /", path);
    return FileImplPtr();
  }

  char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+2);
  if(!temp) {
    log_e("malloc failed");
    return FileImplPtr();
  }

  sprintf(temp,"%s%s", _mountpoint, path);

  struct psramStat_t st;

  if(!pfs_stat(temp, &st)) {
    // file exists
    free(temp);
    if ( st.st_mode == DT_REG || st.st_mode == DT_DIR ) {
      log_d("new PSRamFSFileImpl from existing file %s ", path );
      return std::make_shared<PSRamFSFileImpl>(this, path, mode);
    }
    log_e("new FileImplPtr from %s wrong mode 0x%08X", path, st.st_mode);
    return FileImplPtr();
  }

  //file not found but mode permits creation
  if(mode && mode[0] != 'r') {
    free(temp);
    log_d("new PSRamFSFileImpl from unexisting file %s ", path );
    return std::make_shared<PSRamFSFileImpl>(this, path, mode);
  }

  //try to open this as directory (might be mount point)
  PSRAMDIR * d = pfs_opendir(temp);
  if(d) {
    pfs_closedir(d);
    free(temp);
    log_d("new PSRamFSFileImpl from mount point %s ", path );
    return std::make_shared<PSRamFSFileImpl>(this, path, mode);
  }
  log_e("new FileImplPtr from %s file not found error", temp);
  free(temp);
  return FileImplPtr();
}


bool PSRamFSImpl::exists(const char* path)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return false;
  }

  PSRamFSFileImpl f(this, path, "r");
  if(f) {
    f.close();
    return true;
  }
  return false;
}


bool PSRamFSImpl::rename(const char* pathFrom, const char* pathTo)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return false;
  }

  if(!pathFrom || pathFrom[0] != '/' || !pathTo || pathTo[0] != '/') {
    log_e("bad arguments");
    return false;
  }
  if(!exists(pathFrom)) {
    log_e("%s does not exists", pathFrom);
    return false;
  }
  char * temp1 = (char *)malloc(strlen(pathFrom)+strlen(_mountpoint)+1);
  if(!temp1) {
    log_e("malloc failed");
    return false;
  }
  char * temp2 = (char *)malloc(strlen(pathTo)+strlen(_mountpoint)+1);
  if(!temp2) {
    free(temp1);
    log_e("malloc failed");
    return false;
  }
  sprintf(temp1,"%s%s", _mountpoint, pathFrom);
  sprintf(temp2,"%s%s", _mountpoint, pathTo);
  auto rc = pfs_rename(temp1, temp2);
  free(temp1);
  free(temp2);
  return rc == 0;
}


bool PSRamFSImpl::remove(const char* path)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return false;
  }

  if(!path || path[0] != '/') {
    log_e("bad arguments");
    return false;
  }

  PSRamFSFileImpl f(this, path, "r");
  if(!f || f.isDirectory()) {
    if(f) {
      f.close();
    }
    log_e("%s does not exists or is directory", path);
    return false;
  }
  f.close();

  char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
  if(!temp) {
    log_e("malloc failed");
    return false;
  }
  sprintf(temp,"%s%s", _mountpoint, path);
  auto rc = pfs_unlink(temp);
  free(temp);
  return rc == 0;
}


bool PSRamFSImpl::mkdir(const char *path)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return false;
  }

  PSRamFSFileImpl f(this, path, "r");
  if(f && f.isDirectory()) {
    f.close();
    //log_w("%s already exists", path);
    return true;
  } else if(f) {
    f.close();
    log_e("%s is a file", path);
    return false;
  }

  char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
  if(!temp) {
    log_e("malloc failed");
    return false;
  }
  sprintf(temp,"%s%s", _mountpoint, path);
  auto rc = pfs_mkdir(temp/*, ACCESSPERMS*/);
  free(temp);
  return rc == 0;
}


bool PSRamFSImpl::rmdir(const char *path)
{
  if(!_mountpoint) {
    log_e("File system is not mounted");
    return false;
  }

  if (strcmp(_mountpoint, "/spiffs") == 0) {
    log_e("rmdir is unnecessary in SPIFFS");
    return false;
  }

  PSRamFSFileImpl f(this, path, "r");
  if(!f || !f.isDirectory()) {
    if(f) {
      f.close();
    }
    log_e("%s does not exists or is a file", path);
    return false;
  }
  f.close();

  char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
  if(!temp) {
    log_e("malloc failed");
    return false;
  }
  sprintf(temp,"%s%s", _mountpoint, path);
  auto rc = ::rmdir(temp);
  free(temp);
  return rc == 0;
}




PSRamFSFileImpl::PSRamFSFileImpl(PSRamFSImpl* fs, const char* path, const char* mode)
  : _fs(fs)
  , _f(NULL)
  , _d(NULL)
  , _path(NULL)
  , _isDirectory(false)
  , _written(false)
{
  char * temp = (char *)malloc(strlen(path)+strlen(_fs->_mountpoint)+1);
  if(!temp) {
    return;
  }
  sprintf(temp,"%s%s", _fs->_mountpoint, path);

  _path = strdup(path);
  if(!_path) {
    log_e("strdup(%s) failed", path);
    free(temp);
    return;
  }

  if(!pfs_stat(temp, &_stat)) {
    //file found
    if ( _stat.st_mode == DT_REG) {
      _isDirectory = false;
      _f = pfs_fopen(temp, mode);
      if(!_f) {
        log_e("pfs_fopen(%s) failed", temp);
      } else {
        log_v("pfs_fopen(%s) success", temp);
      }
    } else if( _stat.st_mode == DT_DIR ) {
      _isDirectory = true;
      _d = pfs_opendir(temp);
      if(!_d) {
        log_e("pfs_opendir(%s) failed", temp);
      } else {
        log_v("pfs_opendir(%s) success", temp);
      }
    } else {
      log_e("Unknown type 0x%08X for file %s", _stat.st_mode, temp);
    }
  } else {
    //file not found
    if(!mode || mode[0] == 'r') {
      //try to open as directory
      _d = pfs_opendir(temp);
      if(_d != NULL) {
        _isDirectory = true;
        log_d("pfs_opendir(%s) == directory", temp);
      } else {
        _isDirectory = false;
        log_w("pfs_opendir(%s) failed", temp);
      }
    } else {
      //lets create this new file
      _isDirectory = false;
      _f = pfs_fopen(temp, mode);
      if(!_f) {
        log_e("pfs_fopen(%s) failed", temp);
      } else {
        log_v("pfs_fopen(%s) success", temp);
      }
    }
  }
  free(temp);
}


PSRamFSFileImpl::~PSRamFSFileImpl()
{
  close();
}

void PSRamFSFileImpl::close()
{
  if(_path) {
    free(_path);
    _path = NULL;
  }
  if(_isDirectory && _d) {
    pfs_closedir(_d);
    _d = NULL;
    _isDirectory = false;
  } else if(_f) {
    pfs_fclose(_f);
    _f = NULL;
  }
}


PSRamFSFileImpl::operator bool()
{
  return (_isDirectory && _d != NULL) || _f != NULL;
}


time_t PSRamFSFileImpl::getLastWrite() {
  _getStat() ;
  return _stat.st_mtime;
}


void PSRamFSFileImpl::_getStat() const
{
  if(!_path) {
    return;
  }
  char * temp = (char *)malloc(strlen(_path)+strlen(_fs->_mountpoint)+1);
  if(!temp) {
    return;
  }
  sprintf(temp,"%s%s", _fs->_mountpoint, _path);
  if(!pfs_stat(temp, &_stat)) {
    _written = false;
  }
  free(temp);
}


size_t PSRamFSFileImpl::write(const uint8_t *buf, size_t size)
{
  if(_isDirectory || !_f || !buf || !size) {
    return 0;
  }
  _written = true;
  return pfs_fwrite(buf, 1, size, _f);
}


size_t PSRamFSFileImpl::read(uint8_t* buf, size_t size)
{
  if(_isDirectory || !_f || !buf || !size) {
    log_e("nothing to read");
    return 0;
  }
  log_v("Reading %d bytes", size);

  return pfs_fread(buf, 1, size, _f);
}


size_t PSRamFSFileImpl::readBytes(char *buffer, size_t length)
{
  return read((uint8_t*)buffer, length);
}


bool PSRamFSFileImpl::available()
{
  log_e("Availability check");
  if( !_f ) {
    return false;
  }
  if( _f->index >= _f->size -1 ) return false;
  return true;
}



int PSRamFSFileImpl::read()
{
  log_w("Reading 1 byte");
  uint8_t buf[2] = {0,0};
  if( read( buf, 1 ) == 1 ) {
    return buf[0];
  }
  return -1;
}


bool PSRamFSFileImpl::print(const char* str)
{
  if( write( (const uint8_t*)str, strlen(str) ) > 0 ) {
    log_d("Written %d bytes: %s", strlen(str), str );
    return true;
  }
  return false;
};




void PSRamFSFileImpl::flush()
{

  if(_isDirectory || !_f) {
    return;
  }
  pfs_fflush(_f);
}

bool PSRamFSFileImpl::seek(uint32_t pos, SeekMode mode)
{
  if(_isDirectory || !_f) {
    return false;
  }
  auto rc = pfs_fseek(_f, pos, mode);
  return rc == 0;
}

size_t PSRamFSFileImpl::position() const
{
  if(_isDirectory || !_f) {
    log_e("no position to provide");
    return 0;
  }
  return _f->index;
  //return pfs_ftell(_f);
}

size_t PSRamFSFileImpl::size() const
{
  if(_isDirectory || !_f) {
    log_e("no size to provide");
    return 0;
  }
  return _f->size;
  /*
  if (_written) {
    _getStat();
  }
  return _stat.st_size;
  */
}

const char* PSRamFSFileImpl::name() const
{
  return (const char*) _path;
}

//to implement
boolean PSRamFSFileImpl::isDirectory(void)
{
  return _isDirectory;
}

FileImplPtr PSRamFSFileImpl::openNextFile(const char* mode)
{
  if(!_isDirectory || !_d) {
    return FileImplPtr();
  }
  struct dirent *file = pfs_readdir(_d);
  if(file == NULL) {
    return FileImplPtr();
  }
  if(file->d_type != DT_REG && file->d_type != DT_DIR) {
    return openNextFile(mode);
  }
  String fname = String(file->d_name);
  String name = String(_path);
  if(!fname.startsWith("/") && !name.endsWith("/")) {
    name += "/";
  }
  name += fname;

  return std::make_shared<PSRamFSFileImpl>(_fs, name.c_str(), mode);
}

void PSRamFSFileImpl::rewindDirectory(void)
{
  if(!_isDirectory || !_d) {
    return;
  }
  pfs_rewinddir(_d);
}
