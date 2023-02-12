/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
//
//#include <stdio.h>
//#include <stdint.h>
//#include "sdkconfig.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_system.h"
//#include "esp_timer.h"
//#include "main.h"
//#include "soc/soc.h"
//
//
///************************************************************************
//* Variables Globales
//************************************************************************/
//#define QUEUE_LENGTH 10
//#define QUEUE_ITEM_SIZE sizeof(int)
//
//QueueHandle_t dataQueue;
//SemaphoreHandle_t xSemaphore_writeSD = NULL;
//SemaphoreHandle_t xSemaphore_queue = NULL;
//
//static const char *TAG = "MAIN "; // Para los mensajes del micro
//
//void IRAM_ATTR guardarDato(void *pvParameters);
//void IRAM_ATTR crearDato(void *pvParameters);
//
///**
// * @brief Función main
// */
//
//void app_main(void){
//
//    defineLogLevels();
//    if( SD_init() != ESP_OK) return;
//    //ESP_ERROR_CHECK(inicializacion_i2c());
//    //ESP_LOGI(TAG, "I2C Inicializado correctamente");
//    //ESP_ERROR_CHECK(start_file_server("/sdcard"));
//
//    xSemaphore_writeSD = xSemaphoreCreateBinary();
//    xSemaphore_queue = xSemaphoreCreateMutex();
//    validateSemaphoresCreateCorrectly();
//    dataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
//    validateQueueCreateCorrectly();
//
//    ESP_LOGI(TAG, "INICIANDO TAREAS");
//
//    TaskHandle_t Handle_tarea_i2c = NULL;
//    TaskHandle_t Handle_guarda_datos = NULL;
//
//    xTaskCreatePinnedToCore(crearDato, "crearDato", 1024 * 16, NULL, tskIDLE_PRIORITY, &Handle_tarea_i2c,1);
//    xTaskCreatePinnedToCore(guardarDato, "guardarDato", 1024 * 16, NULL, tskIDLE_PRIORITY + 1 , &Handle_guarda_datos,0);
//}
//
//
//void defineLogLevels() {
//    esp_log_level_set("MAIN ", ESP_LOG_INFO );
//    esp_log_level_set("MPU6050 ", ESP_LOG_ERROR );
//    esp_log_level_set("TAREAS ", ESP_LOG_INFO );
//    esp_log_level_set("SD_CARD ", ESP_LOG_INFO );
//    esp_log_level_set("WIFI ", ESP_LOG_ERROR );
//    esp_log_level_set("MQTT ", ESP_LOG_INFO );
//    esp_log_level_set("MENSAJES_MQTT ", ESP_LOG_INFO );
//    esp_log_level_set("HTTP_FILE_SERVER ", ESP_LOG_ERROR );
//}
//
//void IRAM_ATTR crearDato(void *pvParameters) {
//    ESP_LOGI(TAG,"crearDato");
//    int item = 1;
//    while (1) {
//        if( xSemaphoreTake(xSemaphore_queue, portMAX_DELAY) == pdTRUE){
//            ESP_LOGI(TAG, "Creo Dato > Tomo semaforo");
//            xQueueSend(dataQueue, &item, portMAX_DELAY);
//            ESP_LOGI(TAG,"Sent item %d", item);
//            item++;
//            xSemaphoreGive(xSemaphore_queue);
//            ESP_LOGI(TAG, "Creo Dato > Libero semaforo");
//        }
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//    }
//}
//
//
//void IRAM_ATTR guardarDato(void *pvParameters) {
//    ESP_LOGI(TAG,"guardarDato");
//
//    vTaskDelay(500 / portTICK_PERIOD_MS);
//
//
//    int item;
//    char data[100];
//    bool newLine = false;
//
//    while (1) {
//        if( xSemaphoreTake(xSemaphore_queue, portMAX_DELAY) == pdTRUE) {
//            ESP_LOGI(TAG, "Guardo Dato > Tomo semaforo");
//            if ( xQueueReceive(dataQueue, &item, portMAX_DELAY) == pdTRUE){
//                ESP_LOGI(TAG, "Received item %d", item);
//                sprintf(data, "Received item %d", item);
//                SD_writeData(data, newLine);
//                newLine = true;
//            }
//            xSemaphoreGive(xSemaphore_queue);
//            ESP_LOGI(TAG, "Guardo Dato > Libero semaforo");
//        }
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//    }
//}
//
//
//esp_err_t validateQueueCreateCorrectly() {
//    if( dataQueue  != NULL){
//        ESP_LOGI(TAG, "COLA CREADA CORECTAMENTE");
//        return ESP_OK;
//    }
//    return ESP_FAIL;
//}
//
//esp_err_t validateSemaphoresCreateCorrectly() {
//    if( xSemaphore_writeSD  != NULL
//        &&  xSemaphore_queue    != NULL ){
//        ESP_LOGI(TAG, "SEMAFOROS CREADOS CORECTAMENTE");
//        return ESP_OK;
//    }
//    return ESP_FAIL;
//}
//