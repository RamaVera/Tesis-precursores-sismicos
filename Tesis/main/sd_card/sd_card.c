/** \file	sd_card.c
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#include "sd_card.h"

const char *TAG = "SD_CARD "; // Para los mensajes de LOG

//#define SD_DEBUG_MODE

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
    #ifdef SD_DEBUG_MODE
        ESP_LOGI(TAG, "Initializing SD card");
    #endif


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
    #ifdef SD_DEBUG_MODE
        ESP_LOGI(TAG, "Filesystem mounted");
    #endif


    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

esp_err_t SD_writeData(char dataAsString[], bool withNewLine){
    FILE *f = fopen(MOUNT_POINT"/data.txt", "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    #ifdef SD_DEBUG_MODE
        ESP_LOGI(TAG, "File open");
    #endif

    if( withNewLine )
        fprintf(f,"%s \n",dataAsString);
    else
        fprintf(f,"%s",dataAsString);

    #ifdef SD_DEBUG_MODE
        ESP_LOGI(TAG, "File written");
    #endif

    fclose(f);
    return ESP_OK;
}

//
//void leer_config_SD (void)
//{
//    FILE *f_config;
//    char buffer[100];
//    char comando[100];
//    char argumento[100];
//
//
//    if ((f_config = fopen(MOUNT_POINT "/CONFIG.TXT", "r")) == NULL) {
//        ESP_LOGI(TAG, "Error! opening config file");
//        // Program exits if the file pointer returns NULL.
//    }
//
//    else{
//        ESP_LOGI(TAG, "Archivo de configuracion config.txt abierto");
//
//        while (fgets(buffer, sizeof(buffer),f_config)) { // Leo una línea de archivo
//
//            sscanf(buffer,"%s \" %[^\"]s", comando, argumento); // Separo comando y argumento
//
//            if(strcmp("wifi_ssid", comando)==0) {
//                ESP_LOGI(TAG, "wifi_ssid configurado");
//                memcpy(datos_config.wifi_ssid,argumento, sizeof(argumento));
////                                strcpy(datos_config.wifi_ssid,argumento);
//                //printf("Argumento: %s \n", datos_config.wifi_ssid );
//            }
//
//            if(strcmp("wifi_password", comando)==0) {
//                ESP_LOGI(TAG, "Password_wifi configurado");
//                memcpy(datos_config.wifi_password,argumento, sizeof(argumento));
//
////                                strcpy(datos_config.wifi_password,argumento);
//                // printf("Argumento %s \n", datos_config.wifi_password );
//            }
//
//            if(strcmp("mqtt_ip_broker", comando)==0) {
//                ESP_LOGI(TAG, "mqtt_ip_broker configurado");
//                strcpy(datos_config.mqtt_ip_broker,argumento);
//                // printf("Argumento %s \n", datos_config.wifi_password );
//            }
//
//            if(strcmp("ip_tictoc_server", comando)==0) {
//                ESP_LOGI(TAG, "ip_tictoc_server configurado");
//                strcpy(datos_config.ip_tictoc_server,argumento);
//                // printf("Argumento %s \n", datos_config.wifi_password );
//            }
//
//            if(strcmp("usuario_mqtt", comando)==0) {
//                ESP_LOGI(TAG, "usuario_mqtt configurado");
//                strcpy(datos_config.usuario_mqtt,argumento);
//                // printf("Argumento %s \n", datos_config.wifi_password );
//            }
//
//            if(strcmp("password_mqtt", comando)==0) {
//                ESP_LOGI(TAG, "password_mqtt configurado");
//                strcpy(datos_config.password_mqtt,argumento);
//                // printf("Argumento %s \n", datos_config.wifi_password );
//            }
//
//            if(strcmp("puerto_mqtt", comando)==0) {
//                ESP_LOGI(TAG, "puerto_mqtt configurado");
//                datos_config.puerto_mqtt = atoi(argumento);
//                //printf("Puerto MQTT: %d \n", datos_config.puerto_mqtt );
//            }
//
//        }
//        fclose(f_config);
//        ESP_LOGI(TAG, "Archivo de configuracion config.txt cerrado");
//    }
//}
