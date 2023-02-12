/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */


#ifndef MAIN_H_
#define MAIN_H_

#include "varias.h"
#include "driver/gpio.h"
#include "soc/soc.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "configuraciones.h"




#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include <stdint.h>             // Algoritmo de sincronismo
#include "freertos/FreeRTOS.h"  // Algoritmo de sincronismo
#include "freertos/task.h"      // Algoritmo de sincronismo
#include "esp_system.h"         // Algoritmo de sincronismo
#include "esp_spi_flash.h"      // Algoritmo de sincronismo
#include "wifi.h"               // Algoritmo de sincronismo


#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"

#include "driver/uart.h"


#include "sd_card/sd_card.h"
#include "mpu_9250/acelerometroI2C.h"
#include "GPIO.h"
#include "file_server.h"


#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "tareas.h"


// Para la publicacion de mensajes por consola
typedef struct mentaje_t {
        bool mensaje_nuevo;
        char mensaje[100];
} mensaje_t;


// Para cargar la configuracion del sistema
typedef struct nodo_config_t {
  unsigned char wifi_ssid[32];
  unsigned char wifi_password[64];
  char mqtt_ip_broker[16];
  char ip_tictoc_server[16];
  char usuario_mqtt[64];
  char password_mqtt[64];
  uint32_t puerto_mqtt;
} nodo_config_t;

// ESTADOS MUESTREO
#define ESTADO_ESPERANDO_MENSAJE_DE_INICIO  0
#define ESTADO_CONFIGURAR_ALARMA_INICIO_A   1
#define ESTADO_CONFIGURAR_ALARMA_INICIO_B   2
#define ESTADO_ESPERANDO_INICIO             3
#define ESTADO_MUESTREANDO                  4
#define ESTADO_FINALIZANDO_MUESTREO         5
#define ESTADO_MUESTREANDO_ASYNC            6


typedef struct muestreo_t {
        uint8_t estado_muestreo;
        int64_t epoch_inicio;  // Epoch (UTC) resolucion en segundos
        uint32_t int_contador_segundos;  // Contador de la duracion del muestreo
        uint32_t nro_muestreo;       // Identificador del muestreo en curso
        uint8_t datos_mpu [CANT_BYTES_LECTURA]; // Lugar donde guardo los datos leidos del mpu
        uint8_t TABLA0[LONG_TABLAS];
        uint8_t TABLA1[LONG_TABLAS];
        uint8_t selec_tabla_escritura;
        uint8_t selec_tabla_lectura;
        uint8_t nro_tabla_guardada;
        uint8_t nro_tabla_enviada;
        uint32_t nro_muestra_en_seg;
        uint32_t nro_muestra_total_muestreo;
        uint32_t nro_archivo;
        uint32_t duracion_muestreo;
        bool flag_tomar_muestra;
        bool flag_muestra_perdida;
        bool flag_tabla_llena;
        bool flag_tabla_perdida;
        bool flag_fin_muestreo;
        uint32_t cant_muestras_perdidas;  // Contador de muestras perdidas en una tabla.
        uint32_t cantidad_de_interrupciones_de_muestreo;
        uint32_t cantidad_de_muestras_leidas;
}muestreo_t;


//////////////////////////////////////////////////////////////////
// Configuracion de los mensajes de log por el puerto serie
/* Valores posibles
  ESP_LOG_NONE → No log output
  ESP_LOG_ERROR → Critical errors, software module can not recover on its own
  ESP_LOG_WARN → Error conditions from which recovery measures have been taken
  ESP_LOG_INFO → Information messages which describe normal flow of events
  ESP_LOG_DEBUG → Extra information which is not necessary for normal use (values, pointers, sizes, etc).
  ESP_LOG_VERBOSE → Bigger chunks of debugging information, or frequent messages which can potentially flood the output.
*/
void defineLogLevels();
esp_err_t validateSemaphoresCreateCorrectly();
esp_err_t validateQueueCreateCorrectly();






#endif /* MAIN_H_ */
