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

sdmmc_card_t *card;

bool SD_isDataTimestamp(int hour, int min, char *token);

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

esp_err_t SD_writeHeaderToSampleFile(char *pathToSave) {
    return SD_writeDataOnSampleFile("Time\t\tAx\t\tAy\t\tAz\t\tADC\t\t", true, pathToSave);
}

//esp_err_t SD_writDataToSampleFile(SD_data_t data,char *pathToSave) {
//    sprintf(data, SD_LINE_PATTERN_WITH_NEW_LINE,
//            sdData.hour,sdData.min,sdData.seconds,
//            sdData.mpuData.Ax,
//            sdData.mpuData.Ay,
//            sdData.mpuData.Az,
//            sdData.adcData.data);
//    return SD_writeDataOnSampleFile("Time\tAx\t\tAy\t\tAz\t\tADC\t\t", true, pathToSave);
//}

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

esp_err_t SD_createSampleFileWithRange(const char *pathToOpen, int startHour, int startMin, int endHour, int endMin, int *lines) {
    char path[50];
    sprintf(path,"%s/data.txt",pathToOpen);
    DEBUG_PRINT_SD(TAG,"%s",path);
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    int startPos = 0, endPos = 0, countLines = 0;
    char line[MAX_LINE_LENGTH];
    char * timeStamp;
    bool startTimeStampFound = false,endTimeStampFound = false;

    while ( fgets(line, sizeof(line), file) ) {
        countLines ++;
        timeStamp = strtok(line, "\t");
        if( !startTimeStampFound && SD_isDataTimestamp(startHour, startMin, timeStamp)){
            startTimeStampFound = true;
            startPos = ftell(file);
        }
        if( !endTimeStampFound && SD_isDataTimestamp(endHour, endMin, timeStamp)){
            endTimeStampFound = true;
            endPos = ftell(file);
        }
        if( startTimeStampFound && endTimeStampFound ){
            DEBUG_PRINT_SD(TAG, "Start and end time found");
            break;
        }
    }

    if (startPos == 0 || endPos == 0) {
        ESP_LOGE(TAG, "Failed to find start/end time");
        return ESP_FAIL;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    char pathTemp[50];
    sprintf(pathTemp,"%s/temp.txt",pathToOpen);
    FILE* temporalFile = fopen(pathTemp, "w");
    if (file == NULL || temporalFile == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading/writing");
        return ESP_FAIL;
    }

    SD_data_t data;
    char * endptr = NULL;
    char * token;
    fseek(file, startPos, SEEK_SET);
    while (ftell(file) != endPos) {
        fgets(line, sizeof(line), file);

        token = strtok(line, ":");
        data.hour = strtol(token, &endptr, 10);
        if( endptr == token ){
            return ESP_FAIL;
        }

        token = strtok(NULL, ":");
        data.min = strtol(token, &endptr, 10);
        if( endptr == token ){
            return ESP_FAIL;
        }

        token = strtok(NULL, ":");
        data.seconds = strtol(token, &endptr, 10);
        if( endptr == token ){
            return ESP_FAIL;
        }

        token = strtok(NULL, "\t");
        data.mpuData.Ax = strtod(token, &endptr);
        if( endptr == token ){
            return ESP_FAIL;
        }

        token = strtok(NULL, "\t");
        data.mpuData.Ay = strtod(token, &endptr);
        if( endptr == token ){
            return ESP_FAIL;
        }
        
        token = strtok(NULL, "\t");
        data.mpuData.Az = strtod(token, &endptr);
        if( endptr == token ){
            return ESP_FAIL;
        }

        token = strtok(NULL, "\t");
        data.adcData.data = strtod(token, &endptr);
        if( endptr == token ){
            return ESP_FAIL;
        }
        fwrite(&data, sizeof(SD_data_t), 1, temporalFile);
    }
    fclose(file);
    fclose(temporalFile);
    *lines = countLines;

    return ESP_OK;
}


esp_err_t SD_getDataFromSampleFile(char *pathToRetrieve, int line, SD_data_t *dataToRetrieve) {
    FILE *file = fopen(pathToRetrieve, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    fseek(file, line * sizeof(SD_data_t), SEEK_SET);
    fread(dataToRetrieve,sizeof (SD_data_t),1,file);

    fclose(file);
    return ESP_OK;
}

bool SD_isDataTimestamp(int hour, int min, char *timeStamp) {
    char copyOfTimeStamp[MAX_LINE_LENGTH];
    strcpy(copyOfTimeStamp, timeStamp);

    char * endptr = NULL;
    char * token = strtok(copyOfTimeStamp, ":");
    if (strtol(token, &endptr, 10) != hour) {
        return false;
    }
    if( endptr == token ){
        return false;
    }

    token = strtok(NULL, ":");

    if (strtol(token, &endptr, 10) >= min) {
        return false;
    }
    if( endptr == token ){
        return false;
    }
    return true;
}

// Configuration files

esp_err_t SD_getDefaultConfigurationParams(config_params_t * configParams) {

    if ( access(MOUNT_POINT"/config.dat", F_OK) != 0 ){
        DEBUG_PRINT_SD(TAG, "Config DAT not found, retrieve from default");

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