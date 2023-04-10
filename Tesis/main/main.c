/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include <stdio.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "soc/soc.h"

#include "main.h"
#include "mpu_9250/mpu9250.h"



/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_LENGTH 10
#define QUEUE_ITEM_SIZE sizeof(MPU9250_t)

TaskHandle_t MPU_ISR = NULL;

bool isReadingData = false;

QueueHandle_t dataQueue;
SemaphoreHandle_t xSemaphore_writeOnSD = NULL;
SemaphoreHandle_t xSemaphore_newDataOnMPU = NULL;
SemaphoreHandle_t xSemaphore_dataIsSavedOnQueue = NULL;

static const char *TAG = "MAIN "; // Para los mensajes del micro

void IRAM_ATTR mpu9250_enableTaskByInterrupt(void* pvParameters);
void IRAM_ATTR mpu9250_task(void *pvParameters);
void IRAM_ATTR sd_task(void *pvParameters);
/**
 * @brief Función main
 */

void app_main(void) {

    defineLogLevels();
    if( SD_init() != ESP_OK) return;
    if (MPU9250_init() != ESP_OK) return;
    if (MPU9250_enableInterruptWith(mpu9250_enableTaskByInterrupt) != ESP_OK) return;

    xSemaphore_newDataOnMPU = xSemaphoreCreateBinary();
    xSemaphore_writeOnSD = xSemaphoreCreateBinary();
    xSemaphore_dataIsSavedOnQueue = xSemaphoreCreateMutex();
    validateSemaphoresCreateCorrectly();

    dataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    validateQueueCreateCorrectly();

    ESP_LOGI(TAG, "INICIANDO TAREAS");

    xTaskCreatePinnedToCore(mpu9250_task, "mpu9250", 1024 * 16, NULL, tskIDLE_PRIORITY+1  , &MPU_ISR, 0);
    xTaskCreatePinnedToCore(sd_task, "sd_task", 1024 * 16, NULL, tskIDLE_PRIORITY+2, NULL,1);
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

void IRAM_ATTR mpu9250_enableTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    ets_printf(">>>>>");
}

void IRAM_ATTR mpu9250_task(void *pvParameters) {
    ESP_LOGI(TAG, "Mpu task init");

    MPU9250_enableInterrupt(true);
    while (1) {
        // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        xSemaphoreTake(xSemaphore_newDataOnMPU, portMAX_DELAY);
        ESP_LOGI(TAG, "MPU Task Global > take semaphore");

        // xSemaphore_queue toma el semaforo para que se acceda a la cola
        if(xSemaphoreTake(xSemaphore_dataIsSavedOnQueue, portMAX_DELAY) == pdTRUE){
            ESP_LOGI(TAG, "MPU Task Queue > take semaphore");

            MPU9250_t data;
            if( MPU9250_ReadAcce(&data)  != ESP_OK){
                ESP_LOGE(TAG, "MPU Task Fail getting data");
                continue;
            }
            #ifdef SD_DEBUG_MODE
                ESP_LOGI(TAG, "MPU Task Received item %02f %02f %02f", data.Ax,data.Ay,data.Az);
            #endif
            xQueueSend(dataQueue, &data, portMAX_DELAY);    ESP_LOGI(TAG,"MPU Task Sent item");
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueue);  ESP_LOGI(TAG,"MPU Task Queue > free semaphore");
        }
        // Avisa que el dato fue leido por lo que baja el pin de interrupcion esperando que se haga un nuevo
        // flanco ascendente
        mpu9250_ready();
    }
}

void IRAM_ATTR sd_task(void *pvParameters) {
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
            ESP_LOGI(TAG, "SD Task Guardo Dato > Tomo semaforo");
            // Si no hay elementos en la cola no tiene sentido esperar a que aparezcan
            if ( xQueueReceive(dataQueue, &item, 0) == pdTRUE){
                ESP_LOGI(TAG, "SD Task Received item");
                sprintf(data, "%02f\t%02f\t%02f\t", item.Ax,item.Ay,item.Az);
                SD_writeData(data, newLine);
            }
            xSemaphoreGive(xSemaphore_dataIsSavedOnQueue);
            ESP_LOGI(TAG, "SD Task Guardo Dato > Libero semaforo");
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