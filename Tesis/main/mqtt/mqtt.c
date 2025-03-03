/** \file	mqtt.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene las funciones de inicializacion y manejo del protocolo mqtt
 */
#include "mqtt.h"

#ifdef MQTT_DEBUG_MODE
#define DEBUG_PRINT_MQTT(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_MQTT(tag, fmt, ...) do {} while (0)
#endif

static const char* root_ca =
		"-----BEGIN CERTIFICATE-----\n"
		"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\n"
		"-----END CERTIFICATE-----\n";

static const char *TAG = "MQTT";
char topicCommands[MAX_WAITING_COMMANDS][MAX_TOPIC_LENGTH];
int waitingCommandsToProcess = 0;
/********************************** MQTT **************************************/

/* Definiciones */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t);
static void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
char * MQTT_ParseEventID(esp_mqtt_event_id_t event_id);

static bool MQTT_isConnected = false;
static esp_mqtt_client_handle_t client;
esp_mqtt_client_handle_t MQTT_getClient(void);


/*******************************************************************************
MQTT_subscribe(): Subscripción al topic especificado.
*******************************************************************************/
void MQTT_subscribe(const char * topic){
  esp_mqtt_client_subscribe(client, topic, 0);
}

/*******************************************************************************
 MQTT_publish(): publica mensaje en el topic especificado.
*******************************************************************************/
void MQTT_publish(const char * topic, const char * mensaje, unsigned int len) {
  /* CON PUBLISH *********************************************************
  esp_mqtt_client_publish(client, topic, data, len, qos, retain) */
  esp_mqtt_client_publish(client, topic, mensaje, (int)len, 0, 0);

  /* CON ENQUEUE ********************************************************
  esp_mqtt_client_enqueue(client, topic, data, len, qos, retain, store);
  //esp_mqtt_client_enqueue(client, topic, mensaje, 0, 0, 0, 1);*/
}


/*******************************************************************************
 MQTT_processTopic(): lee el mensaje MQTT recibido cuando se dispara el evento
 ******************************************************************************/
 void MQTT_processTopic(const char * topic, const char * data){
    /* Acciones a ejecutar para cada topic recibido */
    if (waitingCommandsToProcess <= MAX_WAITING_COMMANDS){
        strcpy(topicCommands[waitingCommandsToProcess], data);
        waitingCommandsToProcess++;
	    DEBUG_PRINT_MQTT(TAG, "Data of topic received and saved: %s - commands len %d", data, waitingCommandsToProcess);
    }else{
	    DEBUG_PRINT_MQTT(TAG, "Data of topic not saved: %s - commands len %d is full", data, waitingCommandsToProcess);
    }

 }


 /*******************************************************************************
  MQTT_init(): Inicialización de MQTT
  ******************************************************************************/

 esp_err_t MQTT_parseParams(mqttParams_t *mqttParams,char *broker, char *port, char *user, char *password ) {
     char * endptr;

     uint32_t num = strtol(port, &endptr, 10);
     if (*endptr != '\0') {
         ESP_LOGE(TAG,"Error Getting MQTT port");
         return ESP_FAIL;
     }

     strcpy (mqttParams->ip_broker, broker);
	 if(strcmp(user, "xxx") == 0 || strcmp(password, "xxx") == 0) {
		 strcpy (mqttParams->user, "");
		 strcpy (mqttParams->password, "");
	 } else {
		 strcpy (mqttParams->user, user);
		 strcpy (mqttParams->password, password);
	 }
     mqttParams->port = num;
	 
	 ESP_LOGI(TAG, "MQTT Params: %s:%d", mqttParams->ip_broker, mqttParams->port);
	 ESP_LOGI(TAG, "MQTT User: %s", mqttParams->user);
	 ESP_LOGI(TAG, "MQTT Password: %s", mqttParams->password);
     return ESP_OK;
 }


esp_err_t MQTT_init(mqttParams_t mqttParam) {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE );
	size_t freeMemoryBefore = esp_get_free_heap_size();
	ESP_LOGI(TAG, "[APP] Free memory before MQTT init: %d bytes", freeMemoryBefore);
	
	esp_mqtt_client_config_t mqtt_cfg_without_user = {
			.host = mqttParam.ip_broker,
			.port = mqttParam.port
	};
	esp_mqtt_client_config_t mqtt_cfg_with_user = {
			.host = mqttParam.ip_broker,
			.port = mqttParam.port,
			.username = mqttParam.user,
			.password = mqttParam.password,
			.transport = MQTT_TRANSPORT_OVER_SSL,
			.cert_pem = root_ca,
			.disable_auto_reconnect = false,      // Activa reconexión automática
	};
	
	if (strlen(mqttParam.user) != 0 && strlen(mqttParam.password) != 0) {
		client = esp_mqtt_client_init(&mqtt_cfg_with_user); //   Creates mqtt client handle based on the configuration.
		ESP_LOGI(TAG, "MQTT client with user and password %s:%s", mqtt_cfg_with_user.username, mqtt_cfg_with_user.password);
		ESP_LOGI(TAG, "Start MQTT server: %s:%d", mqtt_cfg_with_user.host, mqtt_cfg_with_user.port);
		
	} else {
		client = esp_mqtt_client_init(&mqtt_cfg_without_user); //   Creates mqtt client handle based on the configuration.
		ESP_LOGI(TAG, "MQTT client without user and password");
		ESP_LOGI(TAG, "Start MQTT server: %s:%d", mqtt_cfg_without_user.host, mqtt_cfg_without_user.port);
	}
	

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client); // Starts mqtt client with already created client handle.
    vTaskDelay(50 / portTICK_PERIOD_MS); // waiting 50 ms
	
	// Calcular el consumo de memoria
	size_t freeMemoryAfter = esp_get_free_heap_size();
	ESP_LOGI(TAG, "[APP] Free memory after MQTT init: %d bytes", freeMemoryAfter);
	
	size_t memoryConsumed = freeMemoryBefore - freeMemoryAfter;
	ESP_LOGI(TAG, "[APP] Memory consumed by MQTT init: %d bytes", memoryConsumed);
	
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
            MQTT_isConnected = true;
            break;

    case MQTT_EVENT_DISCONNECTED:
	    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            MQTT_isConnected = false;
            break;

    case MQTT_EVENT_SUBSCRIBED:
	    DEBUG_PRINT_MQTT(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_UNSUBSCRIBED:
	    DEBUG_PRINT_MQTT(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_PUBLISHED:
	    DEBUG_PRINT_MQTT(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

    case MQTT_EVENT_DATA:   // Cuando recibo un mensaje a un topic que estoy subcripto.
	    DEBUG_PRINT_MQTT(TAG, "MQTT_EVENT_DATA");

            // obtener topic y mensaje recibido
            char rcv_topic[MAX_TOPIC_LENGTH]="";
            char rcv_message[MAX_MSG_LENGTH]="";
            strncpy(rcv_topic, event->topic, event->topic_len);
            strncpy(rcv_message, event->data, event->data_len);
		    DEBUG_PRINT_MQTT(TAG, "TOPIC RECEIVED: %s", rcv_topic );
		    DEBUG_PRINT_MQTT(TAG, "MESSAGE RECEIVED: %s", rcv_message);
            MQTT_processTopic(rcv_topic, rcv_message);
            break;

    case MQTT_EVENT_ERROR:
	    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            esp_wifi_statis_dump(0xffff);
            break;

    default:
	    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
	DEBUG_PRINT_MQTT(TAG, "Event dispatched from event loop base=%s, event_id=%d - %s", base, event_id, MQTT_ParseEventID(event_id));
    mqtt_event_handler_cb(event_data);
}

bool MQTT_IsConnected(void){
    return MQTT_isConnected;
}

bool MQTT_HasCommandToProcess(){
    return waitingCommandsToProcess > 0 ;
}

void MQTT_GetCommand(char *rawCommand) {
    if (waitingCommandsToProcess > 0){
        waitingCommandsToProcess--;
        strcpy(rawCommand, topicCommands[waitingCommandsToProcess]);
    }else{
        strcpy(rawCommand, "");
    }
    ESP_LOGI(TAG, "Command retrieved: %s - commands still waiting %d", rawCommand, waitingCommandsToProcess);
}

char * MQTT_ParseEventID(esp_mqtt_event_id_t event_id){
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:      return "MQTT_EVENT_CONNECTED";
        case MQTT_EVENT_DISCONNECTED:   return "MQTT_EVENT_DISCONNECTED";
        case MQTT_EVENT_SUBSCRIBED:     return "MQTT_EVENT_SUBSCRIBED";
        case MQTT_EVENT_UNSUBSCRIBED:   return "MQTT_EVENT_UNSUBSCRIBED";
        case MQTT_EVENT_PUBLISHED:      return "MQTT_EVENT_PUBLISHED";
        case MQTT_EVENT_DATA:           return "MQTT_EVENT_DATA";
        case MQTT_EVENT_ERROR:          return "MQTT_EVENT_ERROR";
        default:                        return "MQTT_EVENT_UNKNOWN";
    }
}