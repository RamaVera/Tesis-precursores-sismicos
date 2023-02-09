/** \file	mqtt.h
 *  \brief	Contiene las funciones de inicializacion y manejo del protocolo mqtt
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 */


#ifndef MQTT_H
#define MQTT_H

// Configuracion del MQTT
// #define Id_NODO "nodo_vialidad"
// #define IP_BROKER_MQTT "192.168.0.10"
// #define PUERTO_MQTT 1883
// #define USUARIO_MQTT "usuario"
// #define PASSWD_MQTT  "usuariopassword"


#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "varias.h"

void inicio_mqtt(void);
void analizar_mensaje_mqtt(char * topic, int topic_size, char * mensaje, int mensaje_size);
void subscripciones(esp_mqtt_client_handle_t client);


// Funciones de envío de mensajes
void mensaje_mqtt_error(char * mensaje_error);
void mensaje_mqtt_estado(void);
void mensaje_confirmacion(uint32_t nro);
void mensaje_fin_muestreo(uint32_t nro);

// Analisis de los mensajes

#endif //MQTT_H
