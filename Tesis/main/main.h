/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "freertos/FreeRTOS.h"  // Algoritmo de sincronismo
#include "freertos/task.h"      // Algoritmo de sincronismo

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include "sd_card/sd_card.h"
#include "mpu_9250/mpu9250.h"
#include "button/button.h"


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