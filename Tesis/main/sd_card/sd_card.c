/** \file	sd_card.c
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#include "sd_card.h"

const char *TAG = "SD_CARD "; // Para los mensajes de LOG

#define SD_DEBUG_MODE
#ifdef SD_DEBUG_MODE
#define DEBUG_PRINT_SD(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_SD(tag, fmt, ...) do {} while (0)
#endif

char fileToSaveSamples[MAX_LINE_LENGTH];
char fileToRetrieveSamples[MAX_LINE_LENGTH];

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

esp_err_t SD_writeDataArrayOnSampleFile(SD_data_t dataToSave[], int len, char *pathToSave) {
    SD_sensors_data_t sensorsData[len];
    for( int i=0;i<len;i++){
        memcpy(&sensorsData[i],&dataToSave[i].sensorsData,sizeof(SD_sensors_data_t));
    }

//    char dataAsString[MAX_LINE_LENGTH*10];
//    for(int i=0;i<len;i++){
//        sprintf(dataAsString,"%02d %02d %02d %f %f %f %d \n",
//                dataToSave[i].hour,
//                dataToSave[i].min,
//                dataToSave[i].seconds,
//                dataToSave[i].sensorsData.mpuData.Ax,
//                dataToSave[i].sensorsData.mpuData.Ay,
//                dataToSave[i].sensorsData.mpuData.Az,
//                dataToSave[i].sensorsData.adcData.data);
//    }

    char path[MAX_LINE_LENGTH*2];
    sprintf(path,"%s/%s",pathToSave,fileToSaveSamples);
    DEBUG_PRINT_SD(TAG,"%s",path);
    FILE *f = fopen(path,"a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    DEBUG_PRINT_SD(TAG, "File open");
    fwrite(sensorsData,sizeof (SD_sensors_data_t),len,f);
//    fwrite(dataToSave, sizeof (SD_data_t), len, f);
//    fprintf(f,"%s",dataAsString);
    DEBUG_PRINT_SD(TAG, "File written");
    fclose(f);
    return ESP_OK;
}

esp_err_t SD_getDataFromRetrieveSampleFile(char *pathToRetrieve, SD_sensors_data_t **dataToRetrieve, size_t *totalDataRetrieved) {
    char path[MAX_LINE_LENGTH*2];
    sprintf(path,"%s/%s",pathToRetrieve,fileToRetrieveSamples);
    DEBUG_PRINT_SD(TAG,"%s",path);
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    // Obtener el tamaño total del archivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Calcular la cantidad de elementos a leer
    size_t elementSize = sizeof(SD_sensors_data_t); // Reemplaza TU_TIPO_DE_DATO por el tipo de dato real del archivo
    size_t numElements = fileSize / elementSize;
    *totalDataRetrieved = numElements;
    DEBUG_PRINT_SD(TAG,"Number of elements to retrieve %d",numElements);
    if( numElements == 0  ){
       ESP_LOGE(TAG, "Error reaching borders of file");
        return ESP_FAIL;
    }

    SD_sensors_data_t *dataToRetrieveAux = (SD_sensors_data_t*) malloc(elementSize*numElements);
    if (dataToRetrieveAux == NULL) {
        ESP_LOGE(TAG, "Error allocating memory");
        return ESP_FAIL;
    }

    if( fread(dataToRetrieveAux,elementSize,numElements,file) != numElements){
        ESP_LOGE(TAG, "Error reading file");
        return ESP_FAIL;
    }

    *dataToRetrieve = dataToRetrieveAux;


    fclose(file);
    return ESP_OK;
}

void SD_setSampleFilePath(int hour, int min) {
    sprintf(fileToSaveSamples, "%02d_%02d.txt", hour, min);
}

void SD_setRetrieveSampleFilePath(int hour, int min) {
    sprintf(fileToRetrieveSamples, "%02d_%02d.txt", hour, min);
}

// Configuration files
esp_err_t SD_getConfigurationParams(config_params_t * configParams) {

    if ( access(MOUNT_POINT"/config.dat", F_OK) != 0 ){
        DEBUG_PRINT_SD(TAG, "Config DAT not found, retrieve from default");
        char buffer[1024];

        if (SD_getRawConfigParams(buffer) == ESP_OK ){
            SD_parseRawConfigParams(configParams, buffer);
        } else {
            SD_setFallbackConfigParams(configParams);
        }

        if ( SD_saveLastConfigParams(configParams) != ESP_OK ){
            ESP_LOGE(TAG, "Failed to write dat config file ");
            return ESP_OK;
        }

    } else {
        DEBUG_PRINT_SD(TAG, "Config DAT found, retrieve from DAT");

        if ( SD_readLastConfigParams(configParams) != ESP_OK ){
            ESP_LOGE(TAG, "Failed to read dat config file ");
            return ESP_OK;
        }
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

void SD_setFallbackConfigParams(config_params_t *pParams) {
    strcpy(pParams->wifi_ssid, "xxx");
    strcpy(pParams->wifi_password, "xxx");
    strcpy(pParams->mqtt_ip_broker, "xxx");
    strcpy(pParams->mqtt_user, "xxx");
    strcpy(pParams->mqtt_password, "xxx");
    strcpy(pParams->mqtt_port, "1");
    strcpy(pParams->init_year, "2023");
    strcpy(pParams->init_month, "1");
    strcpy(pParams->init_day, "1");
}

void SD_parseRawConfigParams(config_params_t *configParams, char *buffer) {
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
}

esp_err_t SD_getRawConfigParams(char *buffer) {
    FILE *config_file = fopen(MOUNT_POINT"/config.txt", "r");
    if (config_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    if (fgets(buffer, sizeof(buffer), config_file) == NULL) { // Leer la única línea del archivo
        ESP_LOGE(TAG, "Failed to read file ");
        fclose(config_file);
        return ESP_FAIL;
    }
    fclose(config_file);
    return ESP_OK;
}

esp_err_t SD_saveLastConfigParams(config_params_t * params) {
    FILE *f = fopen(MOUNT_POINT"/config.dat","a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fwrite(params,sizeof(config_params_t),1,f);
    fclose(f);
    return ESP_OK;
}

esp_err_t SD_readLastConfigParams(config_params_t * params) {
    FILE *f = fopen(MOUNT_POINT"/config.dat","r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fread(params,sizeof(config_params_t),1,f);
    fclose(f);
    return ESP_OK;
}