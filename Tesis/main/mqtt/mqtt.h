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
#include <esp_wifi.h>

#define MAX_WAITING_COMMANDS 3
#define MAX_LENGTH_MQTT_PARAM 64
typedef struct mqttParams {
    char ip_broker[MAX_LENGTH_MQTT_PARAM];
    int port;
    char user[MAX_LENGTH_MQTT_PARAM];
    char password[MAX_LENGTH_MQTT_PARAM];
} mqttParams_t;


/* Prototipos */
esp_err_t MQTT_init(mqttParams_t mqttParam);
bool MQTT_IsConnected(void);
bool MQTT_HasCommandToProcess(void);
void MQTT_GetCommand(char *rawCommand);
void MQTT_processTopic(const char * , const char * );
void MQTT_subscribe(const char * );
void MQTT_publish(const char *topic, const char *mensaje, unsigned int len);
/* Definiciones */
esp_err_t MQTT_parseParams ( mqttParams_t *mqttParams, char *broker, char *port, char *user, char *password );

#define MAX_TOPIC_LENGTH  100
#define MAX_MSG_LENGTH  100

#endif  /* MQTT_H_ */
