/** \file	mqtt.c
 *  \brief	Contiene las funciones de inicializacion y manejo del protocolo mqtt
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion del MQTT
 *  https://www.esp8266.com/viewtopic.php?p=91643
 */

#include "mqtt.h"
#include "../main.h"
#include "../tictoc/daemon.h"


static const char *TAG = "MQTT ";


#define MAX_TOPIC_LENGTH  100
#define MAX_MSG_LENGTH  100
#define MAX_ARGS_LENGTH  20
#define MAX_CANT_ARGS  5


const char Topic_InicioMuestreo[] = "control/inicio_muestreo";
const char Topic_InicioMuestreo_async[] = "control/inicio_muestreo_async";
const char Topic_Cancelacion_Muestreo[] = "control/cancelar_muestreo";
const char Topic_Peticion_estado[] = "control/estado";
const char Topic_reiniciar[] = "control/reiniciar";
const char Topic_borrarSD[] = "control/borrarSD";
const char Topic_config_hora[] = "control/config_hora";


//extern int64_t tiempo_inicio;  // Epoch (UTC) resolucion en segundos
extern bool esperando_inicio;
extern muestreo_t Datos_muestreo;
extern TicTocData * ticTocData;

extern nodo_config_t datos_config;


extern char id_nodo[20];
extern char dir_ip[20];
extern wifi_ap_record_t wifidata;


void analizar_mensaje_mqtt(char * topic, int topic_size, char * mensaje, int mensaje_size){

// Le saco la base al topic recibido. Porque eso cambia entre instalaciones.
        char topic_rcv[MAX_TOPIC_LENGTH] = "";
//    memcpy(topic_rcv, topic+strlen(Topic_Base), topic_size - strlen(Topic_Base));

        strncpy(topic_rcv, topic, topic_size);
        /*SEPARO EN ARGUMENTOS*/
        char args_rcv[MAX_CANT_ARGS][MAX_ARGS_LENGTH];
        char msg_rcv[MAX_MSG_LENGTH] = "";
        strncpy(msg_rcv, mensaje, mensaje_size);
        //https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
        char *token;
        const char s[2] = " "; // separador
        token = strtok(msg_rcv, s);
        int nro_arg = 0;
        while( token != NULL ) {
                strcpy(args_rcv[nro_arg], token);
                token = strtok(NULL, s);
                nro_arg++;
        }

        // ESP_LOGI(TAG,"TOPIC= %.*s ", topic_size,  topic_rcv);
        // ESP_LOGI(TAG,"MENSAJE= %.*s ", mensaje_size, mensaje);

        if(strcmp(Topic_InicioMuestreo, topic_rcv)==0) {
                resetea_muestreo();
                cerrar_archivo();
                ESP_LOGI(TAG, "Mensaje de inicio de muestreo recibido");
                Datos_muestreo.epoch_inicio = atoll(args_rcv[0]);
                Datos_muestreo.epoch_inicio = Datos_muestreo.epoch_inicio * 1000000; // Lo paso a microsegundos
                Datos_muestreo.duracion_muestreo = atoi(args_rcv[1])*60;
                Datos_muestreo.nro_muestreo = atoi(args_rcv[2]);
                Datos_muestreo.estado_muestreo = ESTADO_CONFIGURAR_ALARMA_INICIO_A;
                //  Datos_muestreo.contador_segundos = 0; // Reinicio el contador de segundos
                //  Datos_muestreo.nro_muestra_en_seg = 0;
                //  Datos_muestreo.nro_muestra_total_muestreo = 0;


                ESP_LOGI(TAG, "Tiempo de inicio: %llu ",Datos_muestreo.epoch_inicio);
                ESP_LOGI(TAG, "Duracion del muestreo: %d ",Datos_muestreo.duracion_muestreo);
                ESP_LOGI(TAG, "Numero de muestreo : %d ",Datos_muestreo.nro_muestreo);

                mensaje_confirmacion(Datos_muestreo.nro_muestreo);

        }

        else if(strcmp(Topic_InicioMuestreo_async, topic_rcv)==0) {

                resetea_muestreo();
                cerrar_archivo();
                ESP_LOGI(TAG, "Mensaje de inicio asincrónico de muestreo recibido");

                Datos_muestreo.duracion_muestreo = atoi(args_rcv[0])*60;
                Datos_muestreo.nro_muestreo = atoi(args_rcv[1]);
                Datos_muestreo.estado_muestreo = ESTADO_MUESTREANDO_ASYNC;

                ESP_LOGI(TAG, "Duracion del muestreo: %d ",Datos_muestreo.duracion_muestreo);
                ESP_LOGI(TAG, "Numero de muestreo : %d ",Datos_muestreo.nro_muestreo);
                mensaje_confirmacion(Datos_muestreo.nro_muestreo);

        }


        else if(strcmp(Topic_Cancelacion_Muestreo, topic_rcv)==0) {
                ESP_LOGI(TAG, "Mensaje de cancelación de muestreo recibido");
                resetea_muestreo();
                cerrar_archivo();
        }

        else if(strcmp(Topic_Peticion_estado, topic_rcv)==0) {
                if(strcmp("0", args_rcv[0])==0 || strcmp(id_nodo, args_rcv[0])==0 ) {
                        ESP_LOGI(TAG, "Mensaje de peticion de estado recibido");
                        mensaje_mqtt_estado(); // Al conectarse envía un mensaje de estado
                }

        }

        else if(strcmp(Topic_reiniciar, topic_rcv)==0) {
                if(strcmp("0", args_rcv[0])==0 || strcmp(id_nodo, args_rcv[0])==0 ) {
                        ESP_LOGI(TAG, "Mensaje de reinicio recibido");
                        esp_restart(); // Soft reset
                }
        }

        else if(strcmp(Topic_borrarSD, topic_rcv)==0) {
                if(strcmp("0", args_rcv[0])==0 || strcmp(id_nodo, args_rcv[0])==0 ) {
                        ESP_LOGI(TAG, "Mensaje de borrado recibido");
                        borrar_datos_SD();
                        ESP_LOGI(TAG, "Datos borrados");
                }
        }

        else if(strcmp(Topic_config_hora, topic_rcv)==0) {
                if(strcmp("0", args_rcv[0])==0 || strcmp(id_nodo, args_rcv[0])==0 ) {

                        struct timeval current_time1;
                        current_time1.tv_sec = atoll(args_rcv[1]);
                        current_time1.tv_usec = 0;
                        settimeofday(&current_time1, NULL);
                        ESP_LOGI(TAG, "Hora configurada: %lld", atoll(args_rcv[1]));
                }
        }

}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
        esp_mqtt_client_handle_t client = event->client;

        // your_context_t *context = event->context;
        switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
                subscripciones(client); // Despues de conectarme me subscribo a los topics
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
                analizar_mensaje_mqtt(event->topic,event->topic_len, event->data, event->data_len );
                break;

        case MQTT_EVENT_ERROR:
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
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

//esp_mqtt_client_handle_t client;

static esp_mqtt_client_handle_t client;

esp_mqtt_client_handle_t get_mqtt_client_handle(void)
{
        return client;
}


static void mqtt_app_start(void)
{

  char mqtt_server[20] = IP_BROKER_MQTT;
  int mqttPort = PUERTO_MQTT;
  char mqttUser[100] = USUARIO_MQTT;
  char mqttPassword[100] = PASSWD_MQTT;


  strcpy(mqtt_server, datos_config.mqtt_ip_broker);
  strcpy(mqttUser, datos_config.usuario_mqtt);
  strcpy(mqttPassword, datos_config.password_mqtt);
  mqttPort = datos_config.puerto_mqtt;


        esp_mqtt_client_config_t mqtt_cfg = {
                //.uri = CONFIG_BROKER_URL,
                .host= mqtt_server,
                .username = mqttUser,
                .password = mqttPassword,
                .port = mqttPort,
        };



        ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

        client = esp_mqtt_client_init(&mqtt_cfg); //   Creates mqtt client handle based on the configuration.

        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client); // Registro el evento?
        esp_mqtt_client_start(client); // Starts mqtt client with already created client handle.

}


void mensaje_mqtt_estado(void) {
        int msg_id;
//  msg_id = esp_mqtt_client_publish(get_mqtt_client_handle(), "/topic/qos1", "data_3", 0, 1, 0);
        char mensaje[100];
        esp_wifi_sta_get_ap_info(&wifidata); //https://www.esp32.com/viewtopic.php?t=578

        sprintf(mensaje, "%s %s %d nodo_acelerometro", id_nodo, dir_ip, wifidata.rssi);

        if(ticTocReady(ticTocData)) {
                int64_t ttTime_irq;
                ttTime_irq = ticTocTime(ticTocData);
                char mensaje2[100];
                sprintf(mensaje2, " sincronizado %lld",ttTime_irq);
                strncat(mensaje, mensaje2, 90);
        }
        else{
                strncat(mensaje, " no_sincronizado", 90);

                struct timeval current_time2;
                gettimeofday(&current_time2, NULL);
                char mensaje2[100];

                sprintf(mensaje2, " %lld",(uint64_t) 1000000 * current_time2.tv_sec);
                strncat(mensaje, mensaje2, 90);
        }

        if(Datos_muestreo.estado_muestreo==ESTADO_ESPERANDO_MENSAJE_DE_INICIO) {
          strncat(mensaje, " standby", 90);
        }
        else if(Datos_muestreo.estado_muestreo==ESTADO_CONFIGURAR_ALARMA_INICIO_A || Datos_muestreo.estado_muestreo==ESTADO_CONFIGURAR_ALARMA_INICIO_B || Datos_muestreo.estado_muestreo==ESTADO_ESPERANDO_INICIO ){
          strncat(mensaje, " esperando_hora_inicio", 90);
        }
        else{
          strncat(mensaje, " muestreando", 90);
        }



        msg_id = esp_mqtt_client_enqueue(get_mqtt_client_handle(), "nodo/estado", mensaje, 0, 1, 0, 1);
        ESP_LOGI(TAG, "Mensaje de estado publicado, msg_id=%d", msg_id);

}

void mensaje_mqtt_error(char * mensaje_error) {
        int msg_id;
        char mensaje[100];
        sprintf(mensaje, "%s %s %s", id_nodo, dir_ip, mensaje_error);
        msg_id = esp_mqtt_client_enqueue(get_mqtt_client_handle(), "nodo/error", mensaje, 0, 1, 0, 1);
        ESP_LOGI(TAG, "Mensaje de error publicado, msg_id=%d", msg_id);
}

void mensaje_confirmacion(uint32_t nro) {
        int msg_id;
        char mensaje[100];

        sprintf(mensaje, "%s %s %u", id_nodo, dir_ip, nro);
        msg_id = esp_mqtt_client_enqueue(get_mqtt_client_handle(), "nodo/confirmacion", mensaje, 0, 1, 0, 1);

        ESP_LOGI(TAG, "Mensaje de confirmación de inicio publicado, msg_id=%d", msg_id);
}

void mensaje_fin_muestreo(uint32_t nro) {
        int msg_id;
        char mensaje[100];

        sprintf(mensaje, "%s %s %u", id_nodo, dir_ip, nro);
        msg_id = esp_mqtt_client_enqueue(get_mqtt_client_handle(), "nodo/fin", mensaje, 0, 1, 0, 1);

        ESP_LOGI(TAG, "Mensaje fin de muestreo publicado, msg_id=%d", msg_id);
}





void subscripciones(esp_mqtt_client_handle_t client){

        char topic_subscribe[MAX_TOPIC_LENGTH] = "";

        strcat(topic_subscribe, Topic_InicioMuestreo);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_InicioMuestreo_async);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_Cancelacion_Muestreo);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_Peticion_estado);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_reiniciar);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_borrarSD);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string

        strcat(topic_subscribe, Topic_config_hora);
        esp_mqtt_client_subscribe(client, topic_subscribe, 0);
        ESP_LOGI(TAG, "Subscripto %s", topic_subscribe);
        strcpy(topic_subscribe, ""); // Limpio el string
}


void inicio_mqtt(void){

        mqtt_app_start();
}
