#include "init.h"
#include "uv.h"

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle;

/* mains declaration */
void main_fs(void* ignore);
void main_tcp(void* ignore);
void main_timer(void* ignore);
void main_signal(void* ignore);
void main_limits(void* ignore);

void app_main(void)
{
    ESP_LOGI("APP_MAIN", "Beggining app_main");

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init WiFi with static IP
    wifi_init(SERVERIP);

    // Mount FAT-VFS
    ESP_LOGI("APP_MAIN", "Mounting FAT filesystem");

    esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "fatvfs");
    esp_err_t err = esp_partition_erase_range(partition,0, 1048576);
    if (err != ESP_OK) {
        ESP_LOGE("APP_MAIN", "Failed to format FATFS (%s)", esp_err_to_name(err));
        return;
    }

    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 5,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    err = esp_vfs_fat_spiflash_mount("/spiflash", "fatvfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE("APP_MAIN", "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    ESP_LOGI("APP_MAIN", "FAT filesystem mounted succesfully");

    vTaskDelay(5000/portTICK_RATE_MS);

    // xTaskCreate(main_signal, "startup", 16384, NULL, 5, NULL);
    // xTaskCreate(main_timer, "startup", 16384, NULL, 5, NULL);
    // xTaskCreate(main_tcp, "startup", 16384, NULL, 5, NULL);
    // xTaskCreate(main_fs, "startup", 32768, NULL, 5, NULL);
    xTaskCreate(main_raft, "startup", 65536, NULL, 5, NULL);

}