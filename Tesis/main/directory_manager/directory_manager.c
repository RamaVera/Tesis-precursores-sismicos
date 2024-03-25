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

char sampleDirectoryPath[MAX_SAMPLE_PATH_LENGTH];
char retrieveSampleDirectoryPath[MAX_SAMPLE_PATH_LENGTH];

static const char *TAG = "DIRECTORY "; // Para los mensajes de LOG



esp_err_t DIR_Exist(char *dirPath) {
    if (access(dirPath, F_OK) != 0 ) {
        ESP_LOGW(TAG, "%s not found", dirPath);
        return ESP_FAIL;
    }
    DEBUG_PRINT_DIR(TAG, "%s already exist", dirPath);
    return ESP_OK;
}

esp_err_t DIR_CreateDirectory(char *directoryPath) {
    if( DIR_Exist(directoryPath) != ESP_OK ){
        if( mkdir(directoryPath, PERM_ADMIN) != 0) {
            ESP_LOGE(TAG,"Error creating directory");
            return ESP_FAIL;
        }
        DEBUG_PRINT_DIR(TAG, "%s directory create", directoryPath);
    }
    return ESP_OK;
}

esp_err_t DIR_getMainSampleDirectory(char * path){
    memcpy(path, sampleDirectoryPath, sizeof(sampleDirectoryPath));
    return ESP_OK;
}

esp_err_t DIR_getRetrieveSampleDirectory(char * path){
    memcpy(path, retrieveSampleDirectoryPath, sizeof(retrieveSampleDirectoryPath));
    return ESP_OK;
}


esp_err_t DIR_setMainSampleDirectory(int year, int month, int day) {

    sprintf(sampleDirectoryPath, "%s/%d-%02d-%02d",SAMPLE_PATH, year+1900, month+1, day);
    if(DIR_CreateDirectory(sampleDirectoryPath) != ESP_OK ){
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t DIR_setRetrieveSampleDirectory(int year, int month, int day) {
    sprintf(retrieveSampleDirectoryPath, "%s/%d-%02d-%02d",SAMPLE_PATH, year, month, day);
    if( DIR_Exist(retrieveSampleDirectoryPath) != ESP_OK ){
        return ESP_FAIL;
    }
    DEBUG_PRINT_DIR(TAG, "%s set to retrieve samples", retrieveSampleDirectoryPath);
    return ESP_OK;
}


void DIR_updateRetrieveSampleDirectory(char *pathToRetrieve, int yearToUpdate, int montToUpdate, int dayToUpdate) {
    DIR_setRetrieveSampleDirectory(yearToUpdate, montToUpdate, dayToUpdate);
    DIR_getRetrieveSampleDirectory(pathToRetrieve);
    DEBUG_PRINT_DIR(TAG, "Retrieve directory %s", pathToRetrieve);
}

void DIR_updateMainSampleDirectory(char *pathToRetrieve, int yearToUpdate, int montToUpdate, int dayToUpdate) {
    DIR_setMainSampleDirectory(yearToUpdate, montToUpdate, dayToUpdate);
    DIR_getMainSampleDirectory(pathToRetrieve);
    DEBUG_PRINT_DIR(TAG, "Main directory %s", pathToRetrieve);
}