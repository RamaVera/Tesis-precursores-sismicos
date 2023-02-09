/** \file	sd_card.c
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "sd_card.h"

const char *TAG = "SD_CARD "; // Para los mensajes de LOG

sdmmc_card_t* card;
const char mount_point[] = MOUNT_POINT;

void inicializacion_tarjeta_SD(void)
{
        esp_err_t ret;

        // Options for mounting the filesystem.
        // If format_if_mount_failed is set to true, SD card will be partitioned and
        // formatted in case when mounting fails.
        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
                .format_if_mount_failed = true,
                .max_files = 5,               // Cantidad máxima de archivos abiertos
                //.allocation_unit_size = 16 * 1024
                .allocation_unit_size = 512
        };
        ESP_LOGI(TAG, "Initializing SD card");

        // Use settings defined above to initialize SD card and mount FAT filesystem.
        // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
        // Please check its source code and implement error recovery when developing
        // production applications.

        ESP_LOGI(TAG, "Using SDMMC peripheral");
        sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // Velocidad por defecto = 20MHz → puede llegar a 40MHz
#ifdef SD_40MHZ
        host.max_freq_khz = 40000;
#endif

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

        // To use 1-line SD mode, uncomment the following line:
        slot_config.width = 1;

        // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
        // Internal pull-ups are not sufficient. However, enabling internal pull-ups
        // does make a difference some boards, so we do that here.
        gpio_set_pull_mode(15, GPIO_PULLUP_ONLY); // CMD, needed in 4- and 1- line modes
        gpio_set_pull_mode(2, GPIO_PULLUP_ONLY); // D0, needed in 4- and 1-line modes
        gpio_set_pull_mode(4, GPIO_PULLUP_ONLY); // D1, needed in 4-line mode only
        gpio_set_pull_mode(12, GPIO_PULLUP_ONLY); // D2, needed in 4-line mode only
        gpio_set_pull_mode(13, GPIO_PULLUP_ONLY); // D3, needed in 4- and 1-line modes

        ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

        if (ret != ESP_OK) {
                if (ret == ESP_FAIL) {
                        ESP_LOGE(TAG, "Failed to mount filesystem. "
                                 "If you want the card to be formatted, set format_if_mount_failed = true.");
                } else {
                        ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                                 "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
                }
                return;
        }

        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);

}

void extraccion_tarjeta_SD(void)
{

        // All done, unmount partition and disable SDMMC or SPI peripheral
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        ESP_LOGI(TAG, "Card unmounted");

}

void borrar_datos_SD (void)
{
        DIR *d;
        struct dirent *dir;
        d = opendir(mount_point);
        if (d) {
                while ((dir = readdir(d)) != NULL) {
                        //  printf("%s\n", dir->d_name);

                        if(strcmp("CONFIG.TXT", dir->d_name )) {
                                f_unlink(dir->d_name);
                        }
                }
                closedir(d);
        }

}



extern nodo_config_t datos_config;

void leer_config_SD (void)
{
        FILE *f_config;
        char buffer[100];
        char comando[100];
        char argumento[100];


        if ((f_config = fopen(MOUNT_POINT "/CONFIG.TXT", "r")) == NULL) {
                ESP_LOGI(TAG, "Error! opening config file");
                // Program exits if the file pointer returns NULL.
        }

        else{
                ESP_LOGI(TAG, "Archivo de configuracion config.txt abierto");

                while (fgets(buffer, sizeof(buffer),f_config)) { // Leo una línea de archivo

                        sscanf(buffer,"%s \" %[^\"]s", comando, argumento); // Separo comando y argumento

                        if(strcmp("wifi_ssid", comando)==0) {
                                ESP_LOGI(TAG, "wifi_ssid configurado");
                                memcpy(datos_config.wifi_ssid,argumento, sizeof(argumento));
//                                strcpy(datos_config.wifi_ssid,argumento);
                                //printf("Argumento: %s \n", datos_config.wifi_ssid );
                        }

                        if(strcmp("wifi_password", comando)==0) {
                                ESP_LOGI(TAG, "Password_wifi configurado");
                                memcpy(datos_config.wifi_password,argumento, sizeof(argumento));

//                                strcpy(datos_config.wifi_password,argumento);
                                // printf("Argumento %s \n", datos_config.wifi_password );
                        }

                        if(strcmp("mqtt_ip_broker", comando)==0) {
                                ESP_LOGI(TAG, "mqtt_ip_broker configurado");
                                strcpy(datos_config.mqtt_ip_broker,argumento);
                                // printf("Argumento %s \n", datos_config.wifi_password );
                        }

                        if(strcmp("ip_tictoc_server", comando)==0) {
                                ESP_LOGI(TAG, "ip_tictoc_server configurado");
                                strcpy(datos_config.ip_tictoc_server,argumento);
                                // printf("Argumento %s \n", datos_config.wifi_password );
                        }

                        if(strcmp("usuario_mqtt", comando)==0) {
                                ESP_LOGI(TAG, "usuario_mqtt configurado");
                                strcpy(datos_config.usuario_mqtt,argumento);
                                // printf("Argumento %s \n", datos_config.wifi_password );
                        }

                        if(strcmp("password_mqtt", comando)==0) {
                                ESP_LOGI(TAG, "password_mqtt configurado");
                                strcpy(datos_config.password_mqtt,argumento);
                                // printf("Argumento %s \n", datos_config.wifi_password );
                        }

                        if(strcmp("puerto_mqtt", comando)==0) {
                                ESP_LOGI(TAG, "puerto_mqtt configurado");
                                datos_config.puerto_mqtt = atoi(argumento);
                                //printf("Puerto MQTT: %d \n", datos_config.puerto_mqtt );
                        }

                }
                fclose(f_config);
                ESP_LOGI(TAG, "Archivo de configuracion config.txt cerrado");
        }
}
