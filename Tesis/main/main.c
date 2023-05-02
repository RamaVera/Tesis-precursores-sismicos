/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include "main.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(tag, fmt, ...) do {} while (0)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) do {} while (0)
#endif
/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_LENGTH 50
#define QUEUE_ITEM_SIZE sizeof(MPU9250_t)


TaskHandle_t MPU_ISR = NULL;
TaskHandle_t MPU_CALIB_ISR = NULL;
TaskHandle_t SD_ISR = NULL;
TaskHandle_t WIFI_PUBLISH_ISR = NULL;

QueueHandle_t dataQueueForSD;
QueueHandle_t dataQueueForMQTT;
SemaphoreHandle_t xSemaphore_writeOnSD = NULL;
SemaphoreHandle_t xSemaphore_newDataOnMPU = NULL;
SemaphoreHandle_t xSemaphore_dataIsSavedOnQueueForSD = NULL;
SemaphoreHandle_t xSemaphore_dataIsSavedOnQueueForMQTT = NULL;


bool calibrationDone = false;

static const char *TAG = "MAIN "; // Para los mensajes del micro

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);

void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters);
void IRAM_ATTR mpu9250_readingTask(void *pvParameters);
void IRAM_ATTR sd_savingTask(void *pvParameters);
void IRAM_ATTR wifi_publishDataTask(void *pvParameters);

void printStatus(status_t nextStatus);

/**
 * @brief Función main
 */

void app_main(void) {

    defineLogLevels();
    status_t nextStatus = INITIATING;
    while(nextStatus != DONE){
        printStatus(nextStatus);

        switch (nextStatus) {
            case INITIATING: {
                if (Button_init() != ESP_OK) return;
                if (SD_init() != ESP_OK) return;
                if (MPU9250_init() != ESP_OK) return;
                if (WIFI_init() != ESP_OK) return;
                if (MPU9250_attachInterruptWith(mpu9250_enableReadingTaskByInterrupt, false) != ESP_OK) return;
                if (ESP32_initSemaphores() != ESP_OK) return;
                if (ESP32_initQueue() != ESP_OK) return;
                if (WIFI_connect() == ESP_OK ) {
                    if (MQTT_init() != ESP_OK) return;
                }
                nextStatus = WAITING_TO_INIT;
                break;
            }
            case WAITING_TO_INIT: {
                static bool firstTime = true;
                if (firstTime) {
                    WDT_disableOnAllCores();
                    firstTime = false;
                }
                if (Button_isPressed()) {
                    WDT_enableOnAllCores();
                    nextStatus = SETTING_UP;
                }
                break;
            }
            case SETTING_UP: {
                static bool creatingTask = true;
                if (creatingTask) {
                    creatingTask = false;
                    xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_ISR, 0);
                    xTaskCreatePinnedToCore(mpu9250_calibrationTask, "mpu9250_calibrationTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &MPU_CALIB_ISR, 1);
                    MPU9250_enableInterrupt(true);
                }

                bool firstTimeForCalibration = true;

                if( calibrationDone && firstTimeForCalibration) {
                    firstTimeForCalibration = false;
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    MPU9250_enableInterrupt(false);
                    WDT_removeTask(MPU_ISR);
                    WDT_removeTask(MPU_CALIB_ISR);
                    vTaskDelete(MPU_ISR);
                    vTaskDelete(MPU_CALIB_ISR);

                    DEBUG_PRINT(TAG,"---------------Finish Calibration--------------");
                    nextStatus = INIT_SAMPLING;
                }
                break;
            }
            case INIT_SAMPLING: {
                xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_ISR, 1);
                xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, &SD_ISR,0);
                xTaskCreatePinnedToCore(wifi_publishDataTask, "wifi_publishDataTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &WIFI_PUBLISH_ISR, 1);
                MPU9250_enableInterrupt(true);
                nextStatus = DONE;
                break;
            }

            case DONE:
            case ERROR:
            default:
                 break;
        }
    }
}

void printStatus(status_t nextStatus) {
    static status_t lastStatus = ERROR;

    if( lastStatus != nextStatus) {
        DEBUG_PRINT(TAG, "--------------%s-----------------\n",statusAsString[nextStatus]);
        lastStatus = nextStatus;
    }
}

void defineLogLevels() {
    esp_log_level_set("MAIN", ESP_LOG_INFO );
    esp_log_level_set("MPU9250", ESP_LOG_INFO );
    esp_log_level_set("SD_CARD", ESP_LOG_INFO );
    esp_log_level_set("WIFI", ESP_LOG_ERROR );
    esp_log_level_set("MQTT", ESP_LOG_INFO );
    esp_log_level_set("MENSAJES_MQTT", ESP_LOG_INFO );
    esp_log_level_set("HTTP_FILE_SERVER", ESP_LOG_ERROR );
}

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT(">>>>> \n");
}

void IRAM_ATTR mpu9250_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "Mpu task init");
    WDT_addTask(MPU_ISR);
    while (1) {
        vTaskDelay(1);
        // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        if ( xSemaphoreTake(xSemaphore_newDataOnMPU, portMAX_DELAY) != pdTRUE ){
            continue;
        }

        //DEBUG_PRINT(TAG, "MPU Task Global > take semaphore");
        MPU9250_t data;
        if( MPU9250_ReadAcce(&data)  != ESP_OK){
            ESP_LOGE(TAG, "MPU Task Fail getting data");
            xSemaphoreGive(xSemaphore_newDataOnMPU);
            continue;
        }
        DEBUG_PRINT(TAG, "MPU Task Received item %02f %02f %02f", data.Ax,data.Ay,data.Az);

        // xSemaphore_queue toma el semaforo para que se acceda a la cola
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueueForSD, portMAX_DELAY) == pdTRUE){
            xQueueSend(dataQueueForSD, &data, portMAX_DELAY);
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueueForSD);
        }

        if( calibrationDone ){
            if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueueForMQTT, portMAX_DELAY) == pdTRUE){
                xQueueSend(dataQueueForMQTT, &data, portMAX_DELAY);
                xSemaphoreGive(xSemaphore_dataIsSavedOnQueueForMQTT);
            }
        }

        // Avisa que el dato fue leido por lo que baja el pin de interrupcion esperando que se haga un nuevo
        // flanco ascendente
        mpu9250_ready();
        WDT_reset(MPU_ISR);
    }
}

void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters) {

    ESP_LOGI(TAG,"MPU calibration task init");
    WDT_addTask(MPU_CALIB_ISR);

    MPU9250_t item;
    MPU9250_t accumulator;

    while (1) {
        vTaskDelay(1);
        // Comprobar si la cola está llena
        if (uxQueueSpacesAvailable(dataQueueForSD) == 0) {
            if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueueForSD, portMAX_DELAY) == pdTRUE) {
                while (xQueueReceive(dataQueueForSD, &item, 0) == pdTRUE) {
                    accumulator.Ax += item.Ax;
                    accumulator.Ay += item.Ay;
                    accumulator.Az += item.Az;
                }
                accumulator.Ax /= QUEUE_LENGTH;
                accumulator.Ay /= QUEUE_LENGTH;
                accumulator.Az /= QUEUE_LENGTH;

                DEBUG_PRINT(TAG, "MPU Calibration Task %02f %02f %02f", accumulator.Ax,accumulator.Ay,accumulator.Az);

                if( MPU9250_SetCalibrationForAccel(&accumulator) != ESP_OK){
                    ESP_LOGI(TAG,"Calibration Fail");
                }else{
                    ESP_LOGI(TAG,"Calibration Done");
                    MPU9250_enableInterrupt(false);
                }
                vTaskDelay(1000/portTICK_PERIOD_MS);
                calibrationDone = true;
                xSemaphoreGive(xSemaphore_dataIsSavedOnQueueForSD);
            }
        }
        WDT_reset(MPU_CALIB_ISR);
    }
}

void IRAM_ATTR sd_savingTask(void *pvParameters) {

    ESP_LOGI(TAG,"SD task init");
    WDT_addTask(SD_ISR);

    MPU9250_t item;
    char data[100];
    bool newLine = true;

    sprintf(data, "Ax\t\tAy\t\tAz\t\t");
    SD_writeData(data, newLine);

    while (1) {
        vTaskDelay(1);
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueueForSD, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(dataQueueForSD, &item, 0) == pdTRUE){
                DEBUG_PRINT(TAG, "SD Task Received item");
                sprintf(data, "%02f\t%02f\t%02f\t", item.Ax,item.Ay,item.Az);
                SD_writeData(data, newLine);
            }
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueueForSD);
        }
        WDT_reset(SD_ISR);
    }
}

void IRAM_ATTR wifi_publishDataTask(void *pvParameters){
    ESP_LOGI(TAG,"Wifi publish task init");
    WDT_addTask(WIFI_PUBLISH_ISR);
    MPU9250_t item;

    while (1) {
        vTaskDelay(1);
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueueForMQTT, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(dataQueueForMQTT, &item, 0) == pdTRUE){
                DEBUG_PRINT(TAG, "MQTT Task Received item");
                cJSON *jsonAccelData;
                jsonAccelData = cJSON_CreateObject();
                cJSON_AddNumberToObject(jsonAccelData, "accel_x", item.Ax);
                cJSON_AddNumberToObject(jsonAccelData, "accel_y", item.Ay);
                cJSON_AddNumberToObject(jsonAccelData, "accel_z", item.Az);

                char * rendered;
                rendered = cJSON_Print(jsonAccelData);

                MQTT_publish("datos",rendered,strlen(rendered));
                cJSON_Delete(jsonAccelData);
            }
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueueForMQTT);
        }
        WDT_reset(WIFI_PUBLISH_ISR);
    }
}

esp_err_t ESP32_initQueue() {
    dataQueueForSD = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    dataQueueForMQTT = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    if( dataQueueForMQTT    != NULL &&
        dataQueueForSD      != NULL) {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Error Creating queue");
    return ESP_FAIL;
}

esp_err_t ESP32_initSemaphores() {
    xSemaphore_newDataOnMPU = xSemaphoreCreateBinary();
    xSemaphore_writeOnSD = xSemaphoreCreateBinary();
    xSemaphore_dataIsSavedOnQueueForSD = xSemaphoreCreateMutex();
    xSemaphore_dataIsSavedOnQueueForMQTT = xSemaphoreCreateMutex();
    
    if( xSemaphore_writeOnSD                    != NULL &&
        xSemaphore_newDataOnMPU                 != NULL &&
        xSemaphore_dataIsSavedOnQueueForSD      != NULL &&
        xSemaphore_dataIsSavedOnQueueForMQTT    != NULL ){
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Error Creating semaphores");
    return ESP_FAIL;
}