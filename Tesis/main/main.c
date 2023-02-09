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


//////////////////////////////////////////////////////////////////
/**
 * @brief Función main
 */

void app_main(void)
{
// Configuracion de los mensajes de log por el puerto serie
        esp_log_level_set("MAIN ", ESP_LOG_INFO );
        esp_log_level_set("MPU6050 ", ESP_LOG_ERROR );
        esp_log_level_set("TAREAS ", ESP_LOG_INFO );
        esp_log_level_set("SD_CARD ", ESP_LOG_INFO );
        esp_log_level_set("WIFI ", ESP_LOG_ERROR );
        esp_log_level_set("MQTT ", ESP_LOG_INFO );
        esp_log_level_set("MENSAJES_MQTT ", ESP_LOG_INFO );
        esp_log_level_set("HTTP_FILE_SERVER ", ESP_LOG_ERROR );


        /* Valores posibles
           ESP_LOG_NONE → No log output
           ESP_LOG_ERROR → Critical errors, software module can not recover on its own
           ESP_LOG_WARN → Error conditions from which recovery measures have been taken
           ESP_LOG_INFO → Information messages which describe normal flow of events
           ESP_LOG_DEBUG → Extra information which is not necessary for normal use (values, pointers, sizes, etc).
           ESP_LOG_VERBOSE → Bigger chunks of debugging information, or frequent messages which can potentially flood the output.
         */



// Obtencion de la identificación del nodo (string del macadress de la interfaz Wifi)
        uint8_t derived_mac_addr[6] = {0};
        ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
        sprintf(id_nodo, "%x%x%x%x%x%x",
                derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
                derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);
        ESP_LOGI(TAG, "Identificación: %s", id_nodo);

// Primero que nada me conecto a la red
        inicializacion_tarjeta_SD();
        leer_config_SD ();
        connectToWiFi();



// Inicialización de Variables /////////
        resetea_muestreo();
        cerrar_archivo();



// Creo los semaforos que voy a usar////////////////////////////////////////////
        xSemaphore_tomamuestra = xSemaphoreCreateBinary();
        xSemaphore_guardatabla = xSemaphoreCreateBinary();
        xSemaphore_mutex_archivo = xSemaphoreCreateMutex();



        if( xSemaphore_tomamuestra != NULL &&  xSemaphore_guardatabla != NULL &&  xSemaphore_mutex_archivo != NULL)
        {
                ESP_LOGI(TAG, "SEMAFOROS CREADOS CORECTAMENTE");
        }
////////////////////////////////////////////////////////////////////////////////

        inicializacion_gpios();
        ESP_ERROR_CHECK(inicializacion_i2c());
        ESP_LOGI(TAG, "I2C Inicializado correctamente");

        /* Start the file server */
        ESP_ERROR_CHECK(start_file_server("/sdcard"));

// ////// Inicialización del algoritmo de Javier  /////////////////////////
//         TicTocData* ticTocData = malloc(sizeof(TicTocData));
//         setupTicToc(ticTocData, TICTOC_SERVER, TICTOC_PORT);
// ////////////////////////////////////////////////////////////////////////


/* ALGORITMO DE SINCRONISMO*/

        char tictocserver[20] = TICTOC_SERVER;
        strcpy(tictocserver, datos_config.ip_tictoc_server);


/* ------------------------------------------
   Una pausa al inicio
   --------------------------------------------- */
        // printf("ESPERANDO SEÑAL DE INICIO \n");
        // while (gpio_get_level(BOTON_1)) { // Freno todo hasta apretar un boton
        //         esp_task_wdt_reset(); // Esto no detiene el WDT para el idle task que no llega a correr.
        // }
/* ------------------------------------------ */

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// CREO LAS TAREAS DEFINIENDO EN QUE NÚCLEO SE EJECUTAN
// IMPORTANTE: El nombre de la tarea tiene que tener menos de 16 caracteres

        ESP_LOGI(TAG, "INICIANDO TAREAS");

        TaskHandle_t Handle_tarea_i2c = NULL;
        xTaskCreatePinnedToCore(leo_muestras, "leo_muestras", 1024 * 16, (void *)0, 10, &Handle_tarea_i2c,1);
        TaskHandle_t Handle_guarda_datos = NULL;
        xTaskCreatePinnedToCore(guarda_datos, "guarda_datos", 1024 * 16, (void *)0, 9, &Handle_guarda_datos,0);

}



// printf("Inicio fwrite(): %u ", ciclos_inic = xthal_get_ccount());
// ciclos_inic = xthal_get_ccount();
// printf("FIN fwrite(): %u ", ciclos_fin = xthal_get_ccount());
// printf("Duracion fwrite(): %u\n", (uint32_t)(ciclos_fin-ciclos_inic) );
