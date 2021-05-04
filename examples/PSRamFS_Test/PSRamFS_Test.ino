#include "FS.h"
#include "PSRamFS.h"

// You don't really need to format PSRamFS unless previously used
#define FORMAT_PSRAMFS true

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  delay(100);
  log_n("Listing directory: %s", dirname);

  struct PSRAMFILE
  {
    int file_id;
    char * name;
    char * bytes;
    unsigned long size;
    unsigned long memsize;
    unsigned long index;
  };

  PSRAMFILE ** myFiles = (PSRAMFILE **)PSRamFS.getFiles();

  if( myFiles != NULL ) {
    size_t myFilesCount = 256;
    if( myFilesCount > 0 ) {
      for( int i=0; i<myFilesCount; i++ ) {
        if( myFiles[i]->name != NULL ) {
          log_w("Entity #%d : %s / %d / %d", i, myFiles[i]->name, myFiles[i]->size, myFiles[i]->index );
        }
      }
    } else {
      log_w("Directory empty");
    }
  }
/*
  File root = fs.open(dirname);
  if(!root){
    log_n("- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    log_n(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      log_n("  DIR : %s",  file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      log_n("  FILE: %s\t%d", file.name(), file.size());
    }
    file = root.openNextFile();
  }
  */
  log_n("Listing done\n");
}



void readFile(fs::FS &fs, const char * path)
{
  delay(100);
  log_n("Reading file: %s", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    log_e("- failed to open file for reading\n");
    return;
  }

  if( !file.available() ) {
    log_e("- CANNOT read from file");
  } else {
    log_n("- read from file:\n");
    while( file.available() ) {
      int out = file.read();
      log_d( "Out: %02x", out );
      Serial.write(out);
    }
  }
  Serial.println("\n");
  file.close();
  log_n("Read done\n");
}



void writeFile(fs::FS &fs, const char * path, const char * message)
{
  delay(100);

  File file = fs.open(path, FILE_WRITE);

  if(!file){
    log_e("Failed to open file %s for writing", path);
    return;
  } else {
    log_n("Created file: %s", path);
  }
  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ){
    log_n("- file written\n");
  } else {
    log_e("- write failed\n");
  }
  file.close();
}



void appendFile(fs::FS &fs, const char * path, const char * message)
{
  delay(100);
  log_n("Appending to file: %s", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    log_e("- failed to open file for appending");
    return;
  }
  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ){
    log_n("- message appended\n");
  } else {
    log_e("- append failed\n");
  }
  file.close();
}



void renameFile(fs::FS &fs, const char * path1, const char * path2)
{
  delay(100);
  log_n("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    log_n("- file renamed");
  } else {
    log_e("- rename failed");
  }
}



void deleteFile(fs::FS &fs, const char * path)
{
  delay(100);
  log_n("Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    log_n("- file deleted");
  } else {
    log_e("- delete failed");
  }
}



void testFileIO(fs::FS &fs, const char * path)
{
  delay(100);
  log_n("Testing file I/O with %s\r\n", path);

  static uint8_t buf[512];
  size_t len = 0;
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    log_e("- failed to open file for writing");
    return;
  }

  size_t i;
  log_n("- writing" );
  uint32_t start = millis();
  for(i=0; i<1024; i++){
    if ((i & 0x001F) == 0x001F){
      //Serial.printf("\n[%d]", i);
      Serial.print(".");
    }
    file.write(buf, 512);
  }
  Serial.println("");
  uint32_t end = millis() - start;
  log_n(" - %u bytes written in %u ms\r\n", 1024 * 512, end);
  //Serial.print(".");
  file.close();


  File file2 = fs.open(path);
  start = millis();
  end = start;
  i = 0;
  if(file2 && !file2.isDirectory()){
    len = file2.size();
    size_t flen = len;
    start = millis();
    log_n("- reading" );
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file2.read(buf, toRead);
      if ((i++ & 0x001F) == 0x001F){
        Serial.print(".");
      }
      len -= toRead;
    }
    Serial.println("");
    end = millis() - start;
    log_n("- %u bytes read in %u ms\r\n", flen, end);
    file2.close();
  } else {
    log_n("- failed to open file for reading");
  }
}



void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  if(!PSRamFS.begin()){
    log_e("PSRamFS Mount Failed");
    return;
  }


  log_n("Total space: %10u", PSRamFS.totalBytes());
  log_n("Free space: %10u", PSRamFS.freeBytes());

  listDir(PSRamFS, "/", 0);

  writeFile(PSRamFS, "/blah.txt", "BLAH !");
  writeFile(PSRamFS, "/oops.ico", "???????????????");

  listDir(PSRamFS, "/", 0);

  writeFile(PSRamFS, "/hello.txt", "Hello ");
  appendFile(PSRamFS, "/hello.txt", "World!");
  readFile(PSRamFS, "/hello.txt");

  listDir(PSRamFS, "/", 0);

  renameFile(PSRamFS, "/hello.txt", "/foo.txt");
  readFile(PSRamFS, "/foo.txt");
  deleteFile(PSRamFS, "/foo.txt");

  listDir(PSRamFS, "/", 0);

  testFileIO(PSRamFS, "/test.txt");
  log_n("Free space: %10u\n", PSRamFS.freeBytes());

  listDir(PSRamFS, "/", 0);

  deleteFile(PSRamFS, "/test.txt");

  listDir(PSRamFS, "/", 0);

  log_n( "Test complete" );

}


void loop()
{

}
