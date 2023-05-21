/** \file	mqtt.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene las funciones de inicializacion y manejo del protocolo mqtt
 */


#include "wifi.h"
#include "directory_manager.h"
#include "main.h"
#include <esp_wifi.h>
#include "mqtt.h"

static const char *TAG = "MQTT";

/********************************** MQTT **************************************/

/* Definiciones */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t);
static void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);


static esp_mqtt_client_handle_t client;
esp_mqtt_client_handle_t MQTT_getClient(void);

// parámetros Wifi
extern wifi_ap_record_t wifidata;

/*******************************************************************************
MQTT_subscribe(): Subscripción al topic especificado.
*******************************************************************************/
void MQTT_subscribe(const char * topic){

  esp_mqtt_client_subscribe(client, topic, 0);

}

/*******************************************************************************
 MQTT_publish(): publica mensaje en el topic especificado.
*******************************************************************************/
void MQTT_publish(const char * topic, const char * mensaje,int len) {

  /* CON PUPLISH *********************************************************
  esp_mqtt_client_publish(client, topic, data, len, qos, retain) */
  esp_mqtt_client_publish(client, topic, mensaje, len, 0, 0);

  /* CON ENQUEUE ********************************************************
  esp_mqtt_client_enqueue(client, topic, data, len, qos, retain, store);
  //esp_mqtt_client_enqueue(client, topic, mensaje, 0, 0, 0, 1);*/
}


/*******************************************************************************
 MQTT_processTopic(): lee el mensaje MQTT recibido cuando se dispara el evento
 ******************************************************************************/
 void MQTT_processTopic(const char * topic, const char * data){

   /* Acciones a ejecutar para cada topic recibido */

   // Ingresar código aquí

 }


 /*******************************************************************************
  MQTT_init(): Inicialización de MQTT
  ******************************************************************************/

 esp_err_t MQTT_parseParams(char *broker, char *port, char *user, char *password, mqttParams_t *mqttParams) {
     char * endptr;

     uint32_t num = strtol(port, &endptr, 10);
     if (*endptr != '\0') {
         ESP_LOGE(TAG,"Error Getting MQTT port");
         return ESP_FAIL;
     }

     strcpy (mqttParams->ip_broker, broker);
     strcpy (mqttParams->user, user);
     strcpy (mqttParams->password, password);
     mqttParams->port = num;
     return ESP_OK;
 }


esp_err_t MQTT_init(mqttParams_t mqttParam) {

    esp_mqtt_client_config_t mqtt_cfg = {
            .host = mqttParam.ip_broker,
            .port = mqttParam.port
    };
    //mqtt_cfg.username = user;
    //mqtt_cfg.password = password;

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg); //   Creates mqtt client handle based on the configuration.
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client); // Starts mqtt client with already created client handle.
    vTaskDelay(50 / portTICK_PERIOD_MS); // waiting 50 ms
    return ESP_OK;
}

/* handle cliente MQTT ************************************************/
esp_mqtt_client_handle_t MQTT_getClient(void)
{
     return client;
}


/*****************************************************************************/
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    //esp_mqtt_client_handle_t client = event->client;
    client = event->client;

    // your_context_t *context = event->context;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;

    case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

    case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_DATA:   // Cuando recibo un mensaje a un topic que estoy subcripto.
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            // obtener topic y mensaje recibido
            char rcv_topic[MAX_TOPIC_LENGTH]="";
            char rcv_message[MAX_MSG_LENGTH]="";
            strncpy(rcv_topic, event->topic, event->topic_len);
            strncpy(rcv_message, event->data, event->data_len);
            //ESP_LOGI(TAG, "TOPIC RECEIVED: %s", rcv_topic );
            //ESP_LOGI(TAG, "MESSAGE RECEIVED: %s", rcv_message);
            MQTT_processTopic(rcv_topic, rcv_message);
            break;

    case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            esp_wifi_statis_dump(0xffff);
            break;

    default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}
