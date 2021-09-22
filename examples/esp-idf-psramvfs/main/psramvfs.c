/*\

 * PSRAMFS Example

\*/
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "pfs.h"


static const char TAG[] = "psramvfs_main";


void register_psramvfs(void)
{
    pfs_set_partition_size( 0.5 * heap_caps_get_free_size(MALLOC_CAP_SPIRAM) );

    esp_vfs_pfs_conf_t conf = {
        .base_path = "/psram",
        .partition_label = "psram", // ignored ?
        .format_if_mount_failed = false
    };

    esp_err_t err = esp_vfs_pfs_register(&conf);

    if(err != ESP_OK){
        printf( "Mounting PSRAMFS failed! Error: %d\n", err);
        return;
    } else {
        printf( "PSRAMFS mount successful\n" );
    }

}




void test_psramvfs()
{
    const char* file_path = "/psram/blah.txt";
    const char* data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t f_size = strlen(data);
    char * buffer;

    FILE *f = fopen(file_path, "w");

    if (f == NULL) {
        ESP_LOGE( TAG, "Failed to open file %s for writing", file_path);
        return;
    } else {
        printf( "File %s opened for writing\n", file_path );
    }

    fwrite(data, sizeof(char), f_size, f);
    fclose(f);

    printf( "Successfully wrote %d bytes:\n%s\n", f_size, data );


    f = fopen(file_path, "r");

    if (f == NULL) {
        ESP_LOGE( TAG, "Failed to open file %s for reading", file_path );
        return;
    } else {
        printf( "File %s opened for reading\n", file_path );
    }

    // ftell is still broken with psramfs
    //fseek (f , 0 , SEEK_END);
    //f_size = ftell (f);
    //printf( "File size: %d bytes\n", f_size );
    //rewind (f);

    // allocate memory to contain the whole file:
    buffer = (char*) calloc (f_size+1, sizeof(char));
    if (buffer == NULL) {
        ESP_LOGE( TAG, "Unable to alloc %d bytes", (int)f_size+1);
        return;
    }

    size_t result = fread ( buffer, 1, f_size, f );
    if (result != f_size) {
        ESP_LOGE( TAG, "Sizes mismatch (expected %d bytes, got %d bytes)", (int)f_size, result );
        return;
    } else {
        printf( "Successfully read %d bytes: \n%s\n", (int)f_size, (const char*)buffer );
    }

    fclose(f);
    free( buffer );
}




void app_main(void)
{
    //esp_log_level_set("psramvfs_main", ESP_LOG_VERBOSE);
    //esp_log_level_set("esp_psramfs", ESP_LOG_VERBOSE);
    register_psramvfs();
    test_psramvfs();
}
