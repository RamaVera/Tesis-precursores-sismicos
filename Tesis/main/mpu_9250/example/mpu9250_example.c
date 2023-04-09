///*
// * main.c
// *
// *  Created on: Jan 01, 2023
// *      Author: rverag@fi.uba.ar
// */
//#include <stdio.h>
//#include <stdint.h>
//#include "sdkconfig.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_system.h"
//#include "esp_timer.h"
//#include "soc/soc.h"
//
//#include "main.h"
//#include "mpu_9250/mpu9250.h"
//
//
//
///************************************************************************
//* Variables Globales
//************************************************************************/
//#define QUEUE_LENGTH 10
//#define QUEUE_ITEM_SIZE sizeof(int)
//
//TaskHandle_t MPU_ISR = NULL;
//
//bool isReadingData = false;
//
//SemaphoreHandle_t datos_disponibles;
//
//static const char *TAG = "MAIN "; // Para los mensajes del micro
//
//void IRAM_ATTR habilitarLecturaDeDatos(void* pvParameters);
//void IRAM_ATTR mpu9250_task(void *pvParameters);
//
///**
// * @brief Función main
// */
//
//void app_main(void) {
//
//    defineLogLevels();
//    if( SD_init() != ESP_OK) return;
//    if (MPU9250_init() != ESP_OK) return;
//    if (MPU9250_enableInterruptWith(habilitarLecturaDeDatos) != ESP_OK) return;
//
//    datos_disponibles = xSemaphoreCreateBinary();
//    if (datos_disponibles == NULL) {
//        ESP_LOGE(TAG, "Error al crear el semáforo datos_disponibles");
//        return;
//    }
//
//    ESP_LOGI(TAG, "INICIANDO TAREAS");
//
//    xTaskCreatePinnedToCore(mpu9250_task, "mpu9250", 4096 * 12, NULL, tskIDLE_PRIORITY + 1, &MPU_ISR, 0);
//}
//
//void defineLogLevels() {
//    esp_log_level_set("MAIN", ESP_LOG_INFO );
//    esp_log_level_set("MPU9250", ESP_LOG_INFO );
//    esp_log_level_set("TAREAS", ESP_LOG_INFO );
//    esp_log_level_set("SD_CARD", ESP_LOG_INFO );
//    esp_log_level_set("WIFI", ESP_LOG_ERROR );
//    esp_log_level_set("MQTT", ESP_LOG_INFO );
//    esp_log_level_set("MENSAJES_MQTT", ESP_LOG_INFO );
//    esp_log_level_set("HTTP_FILE_SERVER", ESP_LOG_ERROR );
//}
//
//void IRAM_ATTR habilitarLecturaDeDatos(void* pvParameters){
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    xSemaphoreGiveFromISR(datos_disponibles, &xHigherPriorityTaskWoken);
//    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//    ets_printf("-----");
//}
//
//void IRAM_ATTR mpu9250_task(void *pvParameters) {
//    bool isReady;
//    ESP_LOGI(TAG, "mpu task");
//
//    MPU9250_enableInterrupt(true);
//    while (1) {
//        xSemaphoreTake(datos_disponibles, portMAX_DELAY);
//        isReadingData = true;
//        ESP_LOGI(TAG, "ACA se leeria el sensor");
//        isReady = mpu9250_ready();
//        ESP_LOGI(TAG, "Is data ready %s",isReady?"true":"false");
//        vTaskDelay(1000/portTICK_PERIOD_MS);
//        ESP_LOGI(TAG, "ACA se terminaria de leer el sensor \n");
//        isReadingData = false;
//    }
//}