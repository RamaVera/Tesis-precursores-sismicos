/** \file	sd_card.c
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#include "sd_card.h"

const char *TAG = "SD_CARD "; // Para los mensajes de LOG

//#define SD_DEBUG_MODE
#ifdef SD_DEBUG_MODE
#define DEBUG_PRINT_SD(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_SD(tag, fmt, ...) do {} while (0)
#endif


sdmmc_card_t *card;

esp_err_t SD_init(void){
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024
    };
    const char mount_point[] = MOUNT_POINT;
    DEBUG_PRINT_SD(TAG, "Initializing SD card");


    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SD_SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }
    DEBUG_PRINT_SD(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

esp_err_t SD_writeDataOnSampleFile(char dataAsString[], bool withNewLine, char *pathToSave) {
    char path[50];
    sprintf(path,"%s/data.txt",pathToSave);
    DEBUG_PRINT_SD(TAG,"%s",path);
    FILE *f = fopen(path,"a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    DEBUG_PRINT_SD(TAG, "File open");

    if( withNewLine )
        fprintf(f,"%s \n",dataAsString);
    else
        fprintf(f,"%s",dataAsString);

    DEBUG_PRINT_SD(TAG, "File written");

    fclose(f);
    return ESP_OK;
}


esp_err_t SD_getDefaultConfigurationParams(config_params_t *configParams;) {

    FILE *config_file = fopen(MOUNT_POINT"/config.txt", "r");
    if (config_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), config_file) == NULL) { // Leer la única línea del archivo
        ESP_LOGE(TAG, "Failed to read file ");
        fclose(config_file);
        return ESP_FAIL;
    }
    fclose(config_file);

    char *fields[] = {
            configParams->wifi_ssid,
            configParams->wifi_password,
            configParams->mqtt_ip_broker,
            configParams->mqtt_user,
            configParams->mqtt_password,
            configParams->mqtt_port,
            configParams->init_year,
            configParams->init_month,
            configParams->init_day,
            };

    const int num_fields = sizeof(fields)/sizeof(fields[0]);

    char *token = strtok(buffer, " | ");
    int i = 0;
    while (token != NULL && i < num_fields) {
        strcpy(fields[i], token);
        token = strtok(NULL, " | ");
        i++;
    }
    DEBUG_PRINT_SD(TAG,"WIFI SSID: %s", configParams->wifi_ssid);
    DEBUG_PRINT_SD(TAG,"Password: %s", configParams->wifi_password);
    DEBUG_PRINT_SD(TAG,"MQTT IP Broker: %s", configParams->mqtt_ip_broker);
    DEBUG_PRINT_SD(TAG,"MQTT User: %s", configParams->mqtt_user);
    DEBUG_PRINT_SD(TAG,"MQTT Password: %s", configParams->mqtt_password);
    DEBUG_PRINT_SD(TAG,"MQTT Port: %s",  configParams->mqtt_port);
    DEBUG_PRINT_SD(TAG, "Seed Datetime: %s/%s/%s", configParams->init_year, configParams->init_month, configParams->init_day);

    return ESP_OK;
}

void SD_writeHeaderToSampleFile(char *pathToSave) {
    SD_writeDataOnSampleFile("Ax\t\tAy\t\tAz\t\tADC\t\t", true, pathToSave);
}