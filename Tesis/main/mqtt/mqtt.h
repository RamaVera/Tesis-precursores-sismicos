/** \file	mqtt.h
 *  Mar 2022
 *  Maestría en Sistemas Embebidos / Sistemas embebidos distribuidos
  */

#ifndef MQTT_H
#define MQTT_H

#define IP_BROKER_MQTT "broker.emqx.io"
#define PORT_MQTT 1883
#define USER_MQTT
#define PASSWD_MQTT

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

/* Prototipos */
esp_err_t MQTT_init(void);
void MQTT_processTopic(const char * , const char * );
void MQTT_subscribe(const char * );
void MQTT_publish(const char * , const char * ,int);
/* Definiciones */
#define MAX_TOPIC_LENGTH  100
#define MAX_MSG_LENGTH  100

#endif  /* MQTT_H_ */
