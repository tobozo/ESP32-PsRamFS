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

#ifndef _PSRAMFS_IMPL_H_
#define _PSRAMFS_IMPL_H_

#include "FS.h"
#include "FSImpl.h"
extern "C" {
#include "pfs.h"
}


namespace fs
{

  class PSRamFSFileImpl;
  typedef std::shared_ptr<PSRamFSFileImpl> PSRamFSFileImplPtr;

  class PSRamFSImpl : public FSImpl
  {

    protected:
      friend class PSRamFSFileImpl;

    public:
      FileImplPtr open(const char* path, const char* mode);
      bool        exists(const char* path);
      bool        rename(const char* pathFrom, const char* pathTo);
      bool        remove(const char* path);
      bool        mkdir(const char *path);
      bool        rmdir(const char *path);

  };

  class PSRamFSFileImpl : public FileImpl
  {
    protected:
      PSRamFSImpl*               _fs;
      PSRAMFILE *                _f;
      PSRAMDIR *                 _d;
      char *                     _path;
      bool                       _isDirectory;
      mutable struct psramStat_t _stat;
      mutable bool               _written;

      void _getStat() const;

    public:
      PSRamFSFileImpl(PSRamFSImpl* fs, const char* path, const char* mode);
      ~PSRamFSFileImpl();
      size_t      write(const uint8_t *buf, size_t size);
      bool        print(const char* str);
      virtual int         read(); // ?
      size_t      read(uint8_t* buf, size_t size); // ?
      size_t      readBytes(char *buffer, size_t length); // ?
      bool        available(); // ?
      void        flush();
      bool        seek(uint32_t pos, SeekMode mode);
      size_t      position() const;
      size_t      size() const;
      void        close();
      const char* name() const;
      time_t      getLastWrite() ;
      boolean     isDirectory(void);
      FileImplPtr openNextFile(const char* mode);
      void        rewindDirectory(void);
      operator    bool();
  };

}

using fs::PSRamFSFileImpl;

#endif
