/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include "main.h"

/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_LENGTH 50
#define QUEUE_ITEM_SIZE sizeof(MPU9250_t)

TaskHandle_t INIT_ISR = NULL;
TaskHandle_t MPU_ISR = NULL;
TaskHandle_t MPU_CALIB_ISR = NULL;
TaskHandle_t SD_ISR = NULL;

QueueHandle_t dataQueue;
SemaphoreHandle_t xSemaphore_writeOnSD = NULL;
SemaphoreHandle_t xSemaphore_newDataOnMPU = NULL;
SemaphoreHandle_t xSemaphore_dataIsSavedOnQueue = NULL;

bool calibrationDone = false;

static const char *TAG = "MAIN "; // Para los mensajes del micro


void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);
void IRAM_ATTR mpu9250_enableCalibrationTaskByInterrupt(void* pvParameters);

void IRAM_ATTR esp32_initTask(void *pvParameters);
void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters);
void IRAM_ATTR mpu9250_readingTask(void *pvParameters);
void IRAM_ATTR sd_savingTask(void *pvParameters);
/**
 * @brief Función main
 */

void app_main(void) {

    defineLogLevels();
    if( SD_init() != ESP_OK) return;
    if( MPU9250_init() != ESP_OK) return;
    if( Button_init() != ESP_OK) return;
    if( MPU9250_attachInterruptWith (mpu9250_enableReadingTaskByInterrupt, true) != ESP_OK) return;
    //if( Button_attachInterruptWith  (NULL) != ESP_OK) return;

    xSemaphore_newDataOnMPU = xSemaphoreCreateBinary();
    xSemaphore_writeOnSD = xSemaphoreCreateBinary();
    xSemaphore_dataIsSavedOnQueue = xSemaphoreCreateMutex();
    validateSemaphoresCreateCorrectly();

    dataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    validateQueueCreateCorrectly();

    ESP_LOGI(TAG, "INICIANDO TAREAS");

    xTaskCreatePinnedToCore(esp32_initTask, "esp32_initTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 10,&INIT_ISR, 1);

    xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 1, &MPU_ISR, 0);
    vTaskSuspend(MPU_ISR);

    xTaskCreatePinnedToCore(mpu9250_calibrationTask, "mpu9250_calibrationTask", 1024 * 16, NULL, tskIDLE_PRIORITY, &MPU_CALIB_ISR, 0);
    vTaskSuspend(MPU_CALIB_ISR);

    xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, &SD_ISR, 1);
    vTaskSuspend(SD_ISR);

}

void defineLogLevels() {
    esp_log_level_set("MAIN", ESP_LOG_INFO );
    esp_log_level_set("MPU9250", ESP_LOG_INFO );
    esp_log_level_set("TAREAS", ESP_LOG_INFO );
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
#ifdef SD_DEBUG_MODE
    ets_printf(">>>>>");
#endif
}

void IRAM_ATTR mpu9250_enableCalibrationTaskByInterrupt(void* pvParameters){

}

void IRAM_ATTR mpu9250_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "Mpu task init");

    while (1) {
        // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        xSemaphoreTake(xSemaphore_newDataOnMPU, portMAX_DELAY);
        #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG, "MPU Task Global > take semaphore");
        #endif

        // xSemaphore_queue toma el semaforo para que se acceda a la cola
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueue, portMAX_DELAY) == pdTRUE){
            #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG, "MPU Task Queue > take semaphore");
            #endif

            MPU9250_t data;
            if( MPU9250_ReadAcce(&data)  != ESP_OK){
                ESP_LOGE(TAG, "MPU Task Fail getting data");
                continue;
            }
            ESP_LOGI(TAG, "MPU Task Received item %02f %02f %02f", data.Ax,data.Ay,data.Az);
            xQueueSend(dataQueue, &data, portMAX_DELAY);
            #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG,"MPU Task Sent item");
            #endif
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueue);
            #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG,"MPU Task Queue > free semaphore");
            #endif


        }
        // Avisa que el dato fue leido por lo que baja el pin de interrupcion esperando que se haga un nuevo
        // flanco ascendente
        mpu9250_ready();
    }
}

void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters) {
    ESP_LOGI(TAG,"MPU calibration task init");

    MPU9250_t item;
    MPU9250_t accumulator;

    while (1) {
        // Comprobar si la cola está llena
        if (uxQueueSpacesAvailable(dataQueue) == 0) {
            if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueue, portMAX_DELAY) == pdTRUE) {
                while (xQueueReceive(dataQueue, &item, 0) == pdTRUE) {
                    accumulator.Ax += item.Ax;
                    accumulator.Ay += item.Ay;
                    accumulator.Az += item.Az;
                }
                accumulator.Ax /= QUEUE_LENGTH;
                accumulator.Ay /= QUEUE_LENGTH;
                accumulator.Az /= QUEUE_LENGTH;
                ESP_LOGI(TAG, "MPU Calibration Task %02f %02f %02f", accumulator.Ax,accumulator.Ay,accumulator.Az);
                vTaskDelay(20/portTICK_PERIOD_MS);

                if( MPU9250_SetCalibrationForAccel(&accumulator) != ESP_OK){
                    ESP_LOGI(TAG,"Calibration Fail");
                }else{
                    ESP_LOGI(TAG,"Calibration Done");
                }
                calibrationDone = true;
                xSemaphoreGive(xSemaphore_dataIsSavedOnQueue);
            }
        }
    }
}


void IRAM_ATTR esp32_initTask(void *pvParameters) {
    ESP_LOGI(TAG, "Tasks ready to init");

    bool firstTime = true;

    while (1) {
        if( Button_isPressed() && firstTime ) {
            ESP_LOGI(TAG,"--------------Init Calibration-----------------");
            vTaskDelay(10/portTICK_PERIOD_MS);
            firstTime = false;
            vTaskResume(MPU_ISR);
            vTaskResume(MPU_CALIB_ISR);
        }
        if( !Button_isPressed() && calibrationDone ) {
            ESP_LOGI(TAG,"---------------Finish Calibration--------------");
            vTaskDelay(10/portTICK_PERIOD_MS);
            vTaskResume(SD_ISR);
            vTaskDelete(MPU_CALIB_ISR);
            vTaskDelete(INIT_ISR);
        }
    }

}


void IRAM_ATTR sd_savingTask(void *pvParameters) {

    ESP_LOGI(TAG,"SD task init");

    MPU9250_t item;
    char data[100];
    bool newLine = true;

    sprintf(data, "Ax\t\tAy\t\tAz\t\t");
    SD_writeData(data, newLine);

    while (1) {
        // xSemaphore_dataIsSavedOnQueue actua como semaforo mutex para que no
        // se acceda al la cola desde los dos task
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueue, portMAX_DELAY) == pdTRUE) {
            #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG, "SD Task Guardo Dato > Tomo semaforo");
            #endif

            // Si no hay elementos en la cola no tiene sentido esperar a que aparezcan
            if ( xQueueReceive(dataQueue, &item, 0) == pdTRUE){
                #ifdef SD_DEBUG_MODE
                ESP_LOGI(TAG, "SD Task Received item");
                #endif
                sprintf(data, "%02f\t%02f\t%02f\t", item.Ax,item.Ay,item.Az);
                SD_writeData(data, newLine);
            }
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueue);
            #ifdef SD_DEBUG_MODE
            ESP_LOGI(TAG, "SD Task Guardo Dato > Libero semaforo");
            #endif
        }
    }
}


esp_err_t validateQueueCreateCorrectly() {
    if( dataQueue  != NULL){
        ESP_LOGI(TAG, "COLA CREADA CORECTAMENTE");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t validateSemaphoresCreateCorrectly() {
    if( xSemaphore_writeOnSD            != NULL &&
        xSemaphore_dataIsSavedOnQueue   != NULL &&
        xSemaphore_newDataOnMPU         != NULL){
        ESP_LOGI(TAG, "SEMAFOROS CREADOS CORECTAMENTE");
        return ESP_OK;
    }
    return ESP_FAIL;
}