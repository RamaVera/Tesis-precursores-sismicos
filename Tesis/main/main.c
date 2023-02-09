/*
 * main.c
 *
 *  Created on: May 24, 2020
 *      Author: jaatadia@gmail.com
 */
#include <stdio.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_timer.h"

#include "main.h"

#include "wifi.h"

#include "esp_timer.h"
#include "esp_intr_alloc.h"
#include "soc/soc.h"
#include "driver/gpio.h"
#include "varias.h"
#include "tareas.h"


/************************************************************************
* Variables Globales
************************************************************************/
FILE *f_samples = NULL;

SemaphoreHandle_t xSemaphore_tomamuestra = NULL;
SemaphoreHandle_t xSemaphore_guardatabla = NULL;
SemaphoreHandle_t xSemaphore_mutex_archivo = NULL;

uint8_t LED;
mensaje_t mensaje_consola;
muestreo_t Datos_muestreo;

nodo_config_t datos_config;

static const char *TAG = "MAIN "; // Para los mensajes del micro
char id_nodo[20];
volatile char dir_ip[20];


/**
 * @brief Función main
 */

void app_main(void){

    defineLogLevels();
    inicializacion_tarjeta_SD();
    leer_config_SD ();
    connectToWiFi();
    resetea_muestreo();
    cerrar_archivo();
    inicializacion_gpios();
    ESP_ERROR_CHECK(inicializacion_i2c());
    ESP_LOGI(TAG, "I2C Inicializado correctamente");

    /* Start the file server */
    ESP_ERROR_CHECK(start_file_server("/sdcard"));

    xSemaphore_tomamuestra = xSemaphoreCreateBinary();
    xSemaphore_guardatabla = xSemaphoreCreateBinary();
    xSemaphore_mutex_archivo = xSemaphoreCreateMutex();

    if( xSemaphore_tomamuestra      != NULL &&
        xSemaphore_guardatabla      != NULL &&
        xSemaphore_mutex_archivo    != NULL){
                ESP_LOGI(TAG, "SEMAFOROS CREADOS CORECTAMENTE");
        }

    ESP_LOGI(TAG, "INICIANDO TAREAS");

    TaskHandle_t Handle_tarea_i2c = NULL;
    TaskHandle_t Handle_guarda_datos = NULL;


    xTaskCreatePinnedToCore(leo_muestras, "leo_muestras", 1024 * 16, (void *)0, 10, &Handle_tarea_i2c,1);
    xTaskCreatePinnedToCore(guarda_datos, "guarda_datos", 1024 * 16, (void *)0, 9, &Handle_guarda_datos,0);
}



void defineLogLevels() {
    esp_log_level_set("MAIN ", ESP_LOG_INFO );
    esp_log_level_set("MPU6050 ", ESP_LOG_ERROR );
    esp_log_level_set("TAREAS ", ESP_LOG_INFO );
    esp_log_level_set("SD_CARD ", ESP_LOG_INFO );
    esp_log_level_set("WIFI ", ESP_LOG_ERROR );
    esp_log_level_set("MQTT ", ESP_LOG_INFO );
    esp_log_level_set("MENSAJES_MQTT ", ESP_LOG_INFO );
    esp_log_level_set("HTTP_FILE_SERVER ", ESP_LOG_ERROR );
}