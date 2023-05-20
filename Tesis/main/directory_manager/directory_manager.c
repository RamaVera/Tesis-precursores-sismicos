//
// Created by Ramiro on 5/13/2023.
//

#include "directory_manager.h"

#define DIR_DEBUG_MODE
#ifdef DIR_DEBUG_MODE
#define DEBUG_PRINT_DIR(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_DIR(tag, fmt, ...) do {} while (0)
#endif

char dayDirectoryPath[50];

static const char *TAG = "DIRECTORY "; // Para los mensajes de LOG

esp_err_t DIR_createInitialFiles(char *yearSeed, char *monthSeed, char *daySeed) {
    char yearDirectoryPath[30];
    char monthDirectoryPath[40];

    sprintf(yearDirectoryPath, "%s/%s", SAMPLE_PATH, yearSeed);
    if( DIR_CreateDirectory(yearDirectoryPath) != ESP_OK ){
       return ESP_FAIL;
    }
    sprintf(monthDirectoryPath,"%s/%s",yearDirectoryPath,monthSeed);
    if( DIR_CreateDirectory(monthDirectoryPath) != ESP_OK ){
        return ESP_FAIL;
    }

    sprintf(dayDirectoryPath,"%s/%s",monthDirectoryPath,daySeed);
    if( DIR_CreateDirectory(dayDirectoryPath) != ESP_OK ){
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t DIR_CreateDirectory(char *directoryPath) {
    if (access(directoryPath, F_OK) != 0 ) {
        DEBUG_PRINT_DIR(TAG, "%s not found", directoryPath);
        if (mkdir(directoryPath, PERM_ADMIN) != 0) {
            ESP_LOGE(TAG,"Error creating directory");
            return ESP_FAIL;
        }
        DEBUG_PRINT_DIR(TAG, "%s directory create", directoryPath);
    } else {
        ESP_LOGI(TAG, "%s already exist", directoryPath);
    }
    return ESP_OK;
}

esp_err_t DIR_getPathToWrite(char * path){
    memcpy(path,dayDirectoryPath, sizeof(dayDirectoryPath));
    return ESP_OK;
}


