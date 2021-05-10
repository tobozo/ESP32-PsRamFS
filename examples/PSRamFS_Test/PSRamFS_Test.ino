// PSRamFS Example sketch
#include "PSRamFS.h" // https://github.com/tobozo/ESP32-PsRamFS

#define UNIT_TESTS

#ifdef UNIT_TESTS
  #include "tests/vfs_pfs_tests.h"
#endif



void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  delay(100);
  ESP_LOGW(TAG, "Listing directory: %s", dirname);

  // still hacky, directory support is incomplete in the driver
  struct PSRAMFILE
  {
    int file_id;
    char * name;
    char * bytes;
    uint32_t size;
    uint32_t memsize;
    uint32_t index;
    uint32_t flags;
  };

  PSRAMFILE ** myFiles = (PSRAMFILE **)PSRamFS.getFiles();

  if( myFiles != NULL ) {
    size_t myFilesCount = PSRamFS.getFilesCount();
    if( myFilesCount > 0 ) {
      for( int i=0; i<myFilesCount; i++ ) {
        if( myFiles[i]->name != NULL ) {
          ESP_LOGW(TAG, "Entity #%d : %s / %d / %d", i, myFiles[i]->name, myFiles[i]->size, myFiles[i]->index );
        }
      }
    } else {
      ESP_LOGW(TAG, "Directory empty");
    }
  }
  /*
  // keeping this for later, when directory support is acceptable
  File root = fs.open(dirname);
  if(!root){
    ESP_LOGD(TAG, "- failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    ESP_LOGD(TAG, " - not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      ESP_LOGD(TAG, "  DIR : %s",  file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      ESP_LOGD(TAG, "  FILE: %s\t%d", file.name(), file.size());
    }
    file = root.openNextFile();
  }
  */
  ESP_LOGW(TAG, "[PSRamFS Bytes] Used: %d, Free: %d, Total: %d, heap: %d\n", PSRamFS.usedBytes(), PSRamFS.freeBytes(), PSRamFS.totalBytes(), ESP.getFreePsram() ); ;
}



size_t getFileSize(fs::FS &fs, const char * path)
{
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    ESP_LOGE(TAG, "Failed to open file for reading : %s", path);
    return 0;
  }
  size_t fileSize = file.size();
  file.close();
  return fileSize;
}





void readFile(fs::FS &fs, const char * path)
{
  delay(100);
  ESP_LOGW(TAG, "Will open file for reading: %s", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    ESP_LOGE(TAG, "Failed to open file for reading : %s", path);
    return;
  }

  if( !file.available() ) {
    ESP_LOGE(TAG, "CANNOT read from file: %s", path);
  } else {
    ESP_LOGD(TAG, "Will read from file: %s", path);

    delay(100);
    Serial.println();

    int32_t lastPosition = -1;
    while( file.available() ) {
      size_t position = file.position();
      char a = file.read();
      Serial.write( a );
      if( lastPosition == position ) { // uh-oh
        Serial.println("Halting");
        while(1);
        break;
      } else {
        lastPosition = position;
      }
    }
  }
  Serial.println("\n");
  file.close();
  ESP_LOGW(TAG, "Read done: %s", path);
}



void writeFile(fs::FS &fs, const char * path, const char * message)
{
  delay(100);

  ESP_LOGW(TAG, "Will truncate %s using %s mode", path, FILE_WRITE );

  File file = fs.open(path, FILE_WRITE);

  if(!file){
    ESP_LOGE(TAG, "Failed to open file %s for writing", path);
    return;
  } else {
    ESP_LOGD(TAG, "Truncated file: %s", path);
  }
  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ) {
    ESP_LOGD(TAG, "- data written");
  } else {
    ESP_LOGE(TAG, "- write failed\n");
  }
  file.close();
  ESP_LOGW(TAG, "Write done: %s", path);

}



void appendFile(fs::FS &fs, const char * path, const char * message)
{
  delay(100);

  ESP_LOGW(TAG, "Will append %s using %s mode", path, FILE_APPEND );

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    ESP_LOGE(TAG, "- failed to open file for appending");
    return;
  }
  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ){
    ESP_LOGD(TAG, "- message appended");
  } else {
    ESP_LOGE(TAG, "- append failed\n");
  }
  file.close();
  ESP_LOGW(TAG, "Appending done: %s", path);
}



void renameFile(fs::FS &fs, const char * path1, const char * path2)
{
  delay(100);
  ESP_LOGW(TAG, "Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    ESP_LOGD(TAG, "- file renamed");
  } else {
    ESP_LOGE(TAG, "- rename failed");
  }
}



void deleteFile(fs::FS &fs, const char * path)
{
  delay(100);
  ESP_LOGW(TAG, "Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    ESP_LOGD(TAG, "- file deleted");
  } else {
    ESP_LOGE(TAG, "- delete failed");
  }
}



void testFileIO(fs::FS &fs, const char * path)
{
  delay(100);
  ESP_LOGW(TAG, "Testing file I/O with %s\r\n", path);

  static uint8_t buf[512];
  size_t len = 0;
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    ESP_LOGE(TAG, "- failed to open file for writing");
    return;
  }

  size_t i;
  ESP_LOGD(TAG, "- writing" );
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
  ESP_LOGW(TAG, " - %u bytes written in %u ms\r\n", 1024 * 512, end);
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
    ESP_LOGD(TAG, "- reading" );
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
    ESP_LOGW(TAG, "- %u bytes read in %u ms\r\n", flen, end);
    file2.close();
  } else {
    ESP_LOGE(TAG, "- failed to open file for reading");
  }
}


size_t lastCursor = 0;

void seekLog( fs::File &file, const char* strref, size_t offset, int mode = 0 )
{
  size_t index = offset;
  size_t len = strlen(strref);
  if( ! file.seek( index, (fs::SeekMode)mode ) ) {
    ESP_LOGD(TAG, "Seek call failed at offset %d using mode %d", index, mode );
    //return;
  }
  char a = file.read();

  switch( mode ) {
    case 0: /* seek_set */
      index = offset;
    break;
    case 1: /* seek cur */
      if( lastCursor + offset > len ) {
        index = len;
      } else {
        index = lastCursor + offset;
      }
    break;
    case 2: /* seek end */
      if( offset >= len ) {
        index = 0;
      } else {
        index = len - offset;
        if(offset>0) {
          index++;
        }
      }
    break;
    default:
      // uh-oh
      ESP_LOGE(TAG, "Bad mode value: %d, halting", mode);
      while(1);
  }

  if( index == len ) a = 0x00; // EOF

  if( a != strref[index] ) {
    ESP_LOGE(TAG, "Seek test error at offset %d / mode %d / index %d, expected 0x%02x '%s', got 0x%02x '%s'", offset, mode, index, strref[index], String(strref[index]).c_str(), a, String(a).c_str() );
  } else {
    ESP_LOGW(TAG, "Seek test success at offset %d / mode %d, current index=%d", offset, mode, index );
  }

  lastCursor = index;

}



void testSeek(fs::FS &fs)
{
  char message[] = "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789";
  const char* path = "/test-seek.txt";
  size_t len = strlen(message);

  writeFile(PSRamFS, path, message);

  File file = fs.open(path, "r+");
  if(!file || file.isDirectory()){
    ESP_LOGE(TAG, "Failed to open file for reading : %s", path);
    return;
  }

  // success tests
  seekLog( file, message, 0,  2 );  // seek end
  seekLog( file, message, 12, 2 ); // seek end with -12 offset
  // this test is expected to fail, but still set the cursor
  seekLog( file, message, 12345678, 2 ); // seek end (actually seek start with enormous offset)

  int pos = 0;
  file.seek(0);
  bool overflowed = true;
  while( pos < 1000 ) {
    pos++;
    if( !file.seek( 3, (fs::SeekMode)1) ) {
      // should happen at EOF
      overflowed = false;
      break;
    }
  }
  if( !overflowed ) {
    ESP_LOGD(TAG, "seek(seek_cur) successfully found EOF" );
  } else {
    ESP_LOGE(TAG, "seek(seek_cur) failed to find EOF" );
  }

  for(int i=0;i<30;i++) {
    size_t random_index = random( len );
    seekLog( file, message, random_index, 0 );
  }


  for(int i=0;i<30;i++) {
    size_t random_index = rand()%len;
    char random_char = (rand()%(128-32))+31;
    //ESP_LOGE(TAG, "random char : %s", String(random_char).c_str() );
    file.seek( random_index );
    file.write( message[random_index] );
    //message[random_index] = random_char;
    seekLog( file, message, random_index, 2 );
  }

  file.close();
}


void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  #ifdef UNIT_TESTS
    // - testing the vfs layer from pfs.c
    UNITY_BEGIN();

    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram() );

    RUN_TEST(test_can_format_mounted_partition);
    RUN_TEST(test_setup_teardown);

    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram() );

    UNITY_END(); // stop unit testing
  #endif

  // - testing the fs::FS layer from PSRamFS.cpp
  if(!PSRamFS.begin()){
    ESP_LOGE(TAG, "PSRamFS Mount Failed");
    return;
  }

  ESP_LOGD(TAG, "Total space: %10u", PSRamFS.totalBytes());
  ESP_LOGD(TAG, "Free space: %10u", PSRamFS.freeBytes());

  listDir(PSRamFS, "/", 0);

  testSeek(PSRamFS);

  writeFile(PSRamFS, "/blah.txt", "BLAH !");

  ESP_LOGD(TAG, "FileSize: %d", getFileSize(PSRamFS, "/blah.txt") );


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
  ESP_LOGD(TAG, "Free space: %10u\n", PSRamFS.freeBytes());

  listDir(PSRamFS, "/", 0);

  deleteFile(PSRamFS, "/test.txt");

  listDir(PSRamFS, "/", 0);

  ESP_LOGD(TAG,  "Test complete" );

}


void loop()
{

}


