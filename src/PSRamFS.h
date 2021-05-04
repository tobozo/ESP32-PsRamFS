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

#ifndef _PSRAMFS_H_
#define _PSRAMFS_H_

#include "FS.h"

#define FPSRAM_WIPE_FULL 1
#define FPSRAM_PARTITION_LABEL "psram"
#define FPSRAM_PARTITION_SIZE 0.5 // fraction of total psram free when begin() is called


namespace fs
{

class F_PSRam : public FS
{
  public:
    F_PSRam(FSImplPtr impl);
    bool begin(bool formatOnFail=false, const char * basePath="/psram", uint8_t maxOpenFiles=10, const char * partitionLabel = (char*)FPSRAM_PARTITION_LABEL);
    bool format(bool full_wipe = FPSRAM_WIPE_FULL, char* partitionLabel = (char*)FPSRAM_PARTITION_LABEL);
    size_t totalBytes();
    size_t usedBytes();
    size_t freeBytes();
    void end();
    bool exists(const char* path);
    bool exists(const String& path);
    virtual void **getFiles();

  private:
    size_t partitionSize = 0;
};

}

extern fs::F_PSRam PSRamFS;



class RomDiskStream : public Stream
{
  public:

    RomDiskStream( const unsigned char* arr, size_t size ) : my_byte_array(arr), my_byte_array_size(size), my_byte_array_index(0) { }

    int available()
    {
      return (my_byte_array_index < my_byte_array_size ) ? 1 : 0;
    }

    int read()
    {
      if( available() ) return my_byte_array[++my_byte_array_index];
      else return -1;
    }

    size_t readBytes(char *buffer, size_t length)
    {

      if( ( my_byte_array_index + length ) > my_byte_array_size ) {
        log_e("Attempted to read %d bytes, out of bounds at index %d of %d", length, my_byte_array_index, my_byte_array_size );
        if( my_byte_array_index < my_byte_array_size ) {
          length = my_byte_array_size - my_byte_array_index;
        } else {
          return 0;
        }
      }
      memcpy( buffer, &my_byte_array[my_byte_array_index], length );
      my_byte_array_index += length;
      return length;
    }

    int peek()
    {
      return my_byte_array[my_byte_array_index];
    }

    size_t write(uint8_t)
    {
      return 0;
    }

    void flush() { }

  private:

    const unsigned char* my_byte_array;
    size_t   my_byte_array_size;
    uint32_t my_byte_array_index;
};




#endif /* _PSRAMFS_H_ */
