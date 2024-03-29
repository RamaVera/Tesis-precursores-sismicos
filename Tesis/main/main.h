/** \file	main.h
 *  \brief	Contiene las funciones de manejo e inicializacion
 *  Autor: Ramiro Vera
 *  Versión: 1
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "freertos/FreeRTOS.h"  // Algoritmo de sincronismo
#include "freertos/task.h"      // Algoritmo de sincronismo

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "mqtt_client.h"

#include "sd_card/sd_card.h"
#include "mpu_9250/mpu9250.h"
#include "button/button.h"
#include "wifi/wifi.h"
#include "watchdog/watchdog.h"
#include "mqtt/mqtt.h"
#include "adc/adc.h"
#include "data_packets/data_packet.h"
#include "directory_manager/directory_manager.h"
#include "time_manager/time_manager.h"
#include "command/command.h"

#define MAX_CHARS_PER_SAMPLE (25 + 1)
#define TOPIC_TO_RECEIVE_COMMANDS "tesis/commands"
#define TOPIC_TO_PUBLISH_DATA "tesis/data"
#define MIN_SAMPLES_TO_DO_FUSION 10
#define MIN_TICK_DIFF_TO_MATCH 5

typedef enum status_t{
    INIT_CONFIG,
    INIT_SENSORS,
    WAITING_TO_INIT,
    CALIBRATION,
    INIT_SAMPLING,
    DONE,
    ERROR,
}status_t;


typedef enum sample_change_case_t{
    NEW_MINUTE,
    NEW_DAY,
    NO_CHANGE,
}sample_change_case_t;

const char * statusAsString[] = { "START_CONFIG",
                         "INITIATING",
                         "WAITING_TO_INIT",
                         "CALIBRATION",
                         "INIT_SAMPLING",
                         "DONE",
                         "ERROR"};


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
void printStatus(status_t nextStatus);

esp_err_t ESP32_initSemaphores();
esp_err_t ESP32_initQueue();

#endif /* MAIN_H_ */