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

char fileToSaveSamples[MAX_FILE_PATH_LENGTH];
char fileToRetrieveSamples[MAX_FILE_PATH_LENGTH];

sdmmc_card_t *card;

esp_err_t SD_init (void ){
    esp_log_level_set(TAG, ESP_LOG_VERBOSE );

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

esp_err_t SD_writeDataArrayOnSampleFile(SD_time_t dataToSave[], int len, char *pathToSave) {
	return SD_writeDataArrayOnFile( dataToSave, len, pathToSave, fileToSaveSamples );
}

esp_err_t SD_writeDataArrayOnFile ( const SD_time_t *dataToSave, int len, const char *pathToSave, const char *fileToSave ) {
	SD_t sensorsData[MAX_SIZE_DATA_TO_SAVE];
	if ( len > MAX_SIZE_DATA_TO_SAVE ){
	    ESP_LOGE(TAG, "Error: len is greater than %d",MAX_SIZE_DATA_TO_SAVE);
	    return ESP_FAIL;
	}
	
	for( int i=0;i<len;i++){
	    memcpy(&sensorsData[i],&dataToSave[i].sensorsData,sizeof(SD_t));
	}
	
	char path[MAX_COMPLETE_FILE_PATH_LENGTH];
	sprintf( path, "%s/%s", pathToSave, fileToSave );
	
	DEBUG_PRINT_SD( TAG, "%s", path );
	FILE *f = fopen(path,"a");
	if (f == NULL) {
	    ESP_LOGE(TAG, "Failed to open file for writing");
	    return ESP_FAIL;
	}
	fwrite(sensorsData, sizeof (SD_t), len, f);
	
	fclose(f);
	return ESP_OK;
}

esp_err_t SD_readDataFromRetrieveSampleFile (char *pathToRetrieve, SD_t **dataToRetrieve, size_t * numElements, long *fileLine, int * eof ) {
	char path[MAX_COMPLETE_FILE_PATH_LENGTH];
	sprintf(path,"%s/%s",pathToRetrieve,fileToRetrieveSamples);
	DEBUG_PRINT_SD(TAG,"%s",path);
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		return ESP_FAIL;
	}
	
	// Determinar cuántos elementos quedan en el archivo
	fseek(file, 0, SEEK_END);
	long endPos = ftell(file);
	
	if (endPos == -1) {
		ESP_LOGE(TAG, "Error getting file size");
		return ESP_FAIL;
	}
	
	if (*fileLine > endPos) {
		ESP_LOGE(TAG, "Error: fileLine is greater than file size");
		return ESP_FAIL;
	}
	
	if (endPos == *fileLine) {
		*eof = 1;
		*dataToRetrieve = NULL;
		*numElements = 0;
		fclose(file);
		ESP_LOGI(TAG, "End of file reached");
		return ESP_OK;
	}
	
	size_t remainingElements = (endPos - *fileLine) / sizeof(SD_t);
	
	// Calcular la cantidad de elementos a leer
	size_t elementsToRead = remainingElements > MAX_ELEMENTS_TO_READ ? MAX_ELEMENTS_TO_READ:remainingElements;
	size_t elementSize = sizeof(SD_t);
	SD_t *dataToRetrieveAux = (SD_t*) malloc(elementSize * elementsToRead);
	if (dataToRetrieveAux == NULL) {
		ESP_LOGE(TAG, "Error allocating memory %d bytes - free %d",elementSize * elementsToRead, esp_get_free_heap_size());
		return ESP_FAIL;
	}
	
	// Volver a la posición original en el archivo y leer los datos
	fseek(file, *fileLine, SEEK_SET);
	size_t elementsRead = fread(dataToRetrieveAux,elementSize,elementsToRead,file);
	if( elementsRead < elementsToRead && !feof(file)){
		ESP_LOGE(TAG, "Error reading elements from file");
		return ESP_FAIL;
	}
	DEBUG_PRINT_SD(TAG,"EndPos: %ld - FileLine: %ld - Remaining: %d - Read: %d",endPos,*fileLine,remainingElements,elementsRead);
	
	*dataToRetrieve = dataToRetrieveAux;
	*numElements = elementsToRead;
	// Actualizar line con la posición actual del indicador de archivo
	*fileLine = ftell( file);
	
	// Verificar si se ha alcanzado el final del archivo
	*eof = feof( file);
	fclose(file);
	return ESP_OK;
}

void SD_setSampleFilePath ( int hour, int min, char *filenameCreated ) {
    sprintf(fileToSaveSamples, "%02d_%02d.txt", hour, min);
	if (filenameCreated != NULL){
		strcpy(filenameCreated, fileToSaveSamples);
	}
}

void SD_setRetrieveSampleFilePath ( int hour, int min, char *retrieveFileSelected ) {
    sprintf(fileToRetrieveSamples, "%02d_%02d.txt", hour, min);
	if (retrieveFileSelected != NULL){
		strcpy(retrieveFileSelected, fileToRetrieveSamples);
	}
}

esp_err_t SD_deleteRetrieveSampleFile (char *pathToDelete) {
	char path[MAX_COMPLETE_FILE_PATH_LENGTH];
	sprintf(path,"%s/%s",pathToDelete,fileToRetrieveSamples);
	DEBUG_PRINT_SD(TAG,"%s",path);
	return remove(path) == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t SD_getFreeSpace (uint64_t *free_space, uint64_t *total_space) {
	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;
	
	// Obtener información del sistema de archivos
	if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
		// Calcular sectores libres y totales
		fre_sect = fre_clust * fs->csize;
		tot_sect = fs->n_fatent * fs->csize;
		
		// Convertir a bytes
		 *free_space = (uint64_t)fre_sect * 512;
		 *total_space = (uint64_t)tot_sect * 512;
		
		ESP_LOGI(TAG,"Espacio total: %llu bytes\n", *total_space);
		ESP_LOGI(TAG,"Espacio libre: %llu bytes\n", *free_space);
	} else {
		ESP_LOGI(TAG,"Error al obtener espacio libre\n");
		return ESP_FAIL;
	}
	return ESP_OK;
}

// Configuration files
esp_err_t SD_getConfigurationParams(config_params_t * configParams) {

    if ( access(MOUNT_POINT"/config.dat", F_OK) != 0 ){
        DEBUG_PRINT_SD(TAG, "Config DAT not found, retrieve from default");
        char buffer[MAX_LINE_SIZE];

        if (SD_getRawConfigParams(buffer, MAX_LINE_SIZE) == ESP_OK ){
	        DEBUG_PRINT_SD(TAG, "Parsing from config.txt");
            SD_parseRawConfigParams(configParams, buffer);
        } else {
	        DEBUG_PRINT_SD(TAG, "Parsing from fallback");
            SD_setFallbackConfigParams(configParams);
        }

        if ( SD_saveLastConfigParams(configParams) != ESP_OK ){
            ESP_LOGE(TAG, "Failed to write dat config file ");
            return ESP_FAIL;
        }

    } else {
        DEBUG_PRINT_SD(TAG, "Config DAT found, retrieve from DAT");

        if ( SD_readLastConfigParams(configParams) != ESP_OK ){
            ESP_LOGE(TAG, "Failed to read dat config file ");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG,"WIFI SSID: %s", configParams->wifi_ssid);
	ESP_LOGI(TAG,"Password: %s", configParams->wifi_password);
	ESP_LOGI(TAG,"MQTT IP Broker: %s", configParams->mqtt_ip_broker);
	ESP_LOGI(TAG,"MQTT User: %s", configParams->mqtt_user);
	ESP_LOGI(TAG,"MQTT Password: %s", configParams->mqtt_password);
	ESP_LOGI(TAG,"MQTT Port: %s",  configParams->mqtt_port);
	ESP_LOGI(TAG, "Seed Datetime: %s/%s/%s", configParams->init_year, configParams->init_month, configParams->init_day);

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
        DEBUG_PRINT_SD(TAG, "Token: %s", token);
        strcpy(fields[i], token);
        token = strtok(NULL, " | ");
        i++;
    }
}

esp_err_t SD_getRawConfigParams(char *buffer, int size) {
    FILE *config_file = fopen(MOUNT_POINT"/config.txt", "r");
    if (config_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    if (fgets(buffer, size, config_file) == NULL) { // Leer la única línea del archivo
        ESP_LOGE(TAG, "Failed to read file ");
        fclose(config_file);
        return ESP_FAIL;
    }
    DEBUG_PRINT_SD(TAG, "Buffer: %s", buffer);
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