/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include "main.h"



/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_SAMPLE_LENGTH 50
#define QUEUE_MQTT_LENGTH 2000
#define MIN_DELAY_FOR_WHILE (1)

TaskHandle_t SAMPLE_LOG_ISR = NULL;
TaskHandle_t SD_ISR = NULL;
TaskHandle_t SENSOR_ISR = NULL;

TaskHandle_t MQTT_PUBLISH_ISR = NULL;
TaskHandle_t MQTT_RECEIVE_ISR = NULL;
TaskHandle_t TIME_REFRESH_ISR = NULL;

TimerHandle_t samplePeriodHandle;
TimerHandle_t timeRefreshHandle;

QueueHandle_t SDDataQueue;
QueueHandle_t MQTTDataQueue;

SemaphoreHandle_t xSemaphore_newDataOnMPU = NULL;
SemaphoreHandle_t xSemaphore_MPU_ADCMutexQueue = NULL;
SemaphoreHandle_t xSemaphore_MQTTMutexQueue = NULL;
SemaphoreHandle_t xSemaphore_SDMutexQueue = NULL;

static const char *TAG = "MAIN "; // Para los mensajes del micro
static const char *TAG_SENSORS = "SENSORS_MAIN ";
static const char *TAG_MQTT_RECIEVER = "MQTT_RCVR_MAIN ";
static const char *TAG_MQTT_PUBLISHER= "MQTT_PSHR_MAIN ";
static const char *TAG_SD = "SD_MAIN ";

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT_MAIN(tag, fmt, ...) \
    if (strcmp(tag, TAG_SD) == 0 || \
        strcmp(tag, TAG_MQTT_RECIEVER) == 0 || \
        strcmp(tag, TAG_MQTT_PUBLISHER) == 0) { \
        ESP_LOGW(tag, fmt, ##__VA_ARGS__); \
    } else { \
        ESP_LOGI(tag, fmt, ##__VA_ARGS__); \
    }
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_MAIN(tag, fmt, ...) do {} while (0)
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) do {} while (0)
#endif

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);
void IRAM_ATTR time_refreshInternalTimer(void* pvParameters);
void IRAM_ATTR adc_mpu9250_sampleSynchronize(void* pvParameters);

void IRAM_ATTR sampleSynchronizerTask (void *pvParameters );
void IRAM_ATTR sensors_readingTask(void *pvParameters);

void IRAM_ATTR sd_savingTask(void *pvParameters);
void IRAM_ATTR mqtt_publishDataTask(void *pvParameters);
void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters);
void IRAM_ATTR time_internalTimeSync(void *pvParameters);

bool takeSDQueueWhenSamplesAre(int numberOfSamples);
sample_change_case_t analyzeSampleTime(int hour, int min, int seconds);
char * printSampleTimeState(sample_change_case_t state);
void pushSDDataToMQTTQueueRoutine(size_t totalDataRetrieved, const SD_t *sdData, int minute, int hour);

void app_main(void) {

    status_t nextStatus = INIT_MODULES;
    config_params_t params;
    wifiParams_t wifiParams;
    mqttParams_t mqttParams;
    timeInfo_t timeInfo;

    defineLogLevels();
	DEBUG_PRINT_MAIN(TAG, "Core ID: %d", xPortGetCoreID()); // Main Runs on Core 0
    while(nextStatus != DONE){
        printStatus(nextStatus);

        switch (nextStatus) {
	        case INIT_MODULES: {
		        if ( Button_init() != ESP_OK) return;
		        if ( ADC_Init() != ESP_OK) return;
		        if ( MPU9250_init() != ESP_OK) return;
		        if ( ESP32_initSemaphores() != ESP_OK) return;
		        if ( ESP32_initQueue() != ESP_OK) return;
                if ( SD_init() != ESP_OK) return;
                if ( SD_getConfigurationParams(&params) != ESP_OK) return;
	            if ( TIME_parseParams( &timeInfo, params.init_year, params.init_month, params.init_day ) != ESP_OK) return;
		        if ( WIFI_parseParams( &wifiParams, params.wifi_ssid, params.wifi_password ) != ESP_OK) return;
				if ( WIFI_init(wifiParams) != ESP_OK) return;
                nextStatus = (WIFI_connect() == ESP_OK)? INIT_WITH_WIFI : INIT_SAMPLING;
                break;
            }

            case INIT_WITH_WIFI: {
				// Synchronize time with NTP server
	            if ( TIME_synchronizeTimeAndDateFromInternet() != ESP_OK) {
		            ESP_LOGE(TAG, "Error synchronizing time");
		            break;
	            }
				
				// Timer to refresh time
	            if ( TIMER_create( "timer_refresher", TIMER_TO_REFRESH_HOUR_PERIOD_MS, time_refreshInternalTimer, &timeRefreshHandle) != ESP_OK) return;
	            if ( TIMER_start(timeRefreshHandle) != ESP_OK) return;
	
				// Update SD config with time and date
	            TIME_getInfoTime(&timeInfo);
	            TIME_updateParams(timeInfo, params.init_year, params.init_month, params.init_day);
	            SD_saveLastConfigParams(&params);
				
	            if ( MQTT_parseParams(
		                &mqttParams,
		                params.mqtt_ip_broker,
		                params.mqtt_port,
		                params.mqtt_user,
		                params.mqtt_password) != ESP_OK)
                    return;
                if (MQTT_init(mqttParams) != ESP_OK) break;
                MQTT_subscribe(TOPIC_TO_RECEIVE_COMMANDS);
	            vTaskDelay(1000 / portTICK_PERIOD_MS);
				nextStatus = INIT_WIFI_FUNCTIONS;
	            break;
            }

	        case INIT_WIFI_FUNCTIONS: {
		        // CORE 0
		       // xTaskCreatePinnedToCore(time_internalTimeSync, "time_internalTimeSync", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &TIME_REFRESH_ISR, 0);
		        xTaskCreatePinnedToCore(mqtt_receiveCommandTask, "mqtt_receiveCommandTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 5, &MQTT_RECEIVE_ISR, 0);
		        xTaskCreatePinnedToCore(mqtt_publishDataTask, "mqtt_publishDataTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MQTT_PUBLISH_ISR, 0);
		
		        nextStatus = INIT_SAMPLING;
		        break;
	        }
			
            case INIT_SAMPLING: {
	            TIME_printTimeAndDate(&timeInfo);
	            if (DIR_setMainSampleDirectory(timeInfo.tm_year, timeInfo.tm_mon, timeInfo.tm_mday) != ESP_OK) return;
	
	            // CORE 0
	            xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 6, &SD_ISR,0);
	
	            // CORE 1
	            xTaskCreatePinnedToCore(sampleSynchronizerTask, "sample_synchronizerTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 4, &SAMPLE_LOG_ISR, 1);
	            xTaskCreatePinnedToCore(sensors_readingTask, "sensors_readingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 3, &SENSOR_ISR, 1);
	
	            //xTaskCreatePinnedToCore(adc_readingTask, "adc_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &ADC_ISR, 1);
                //xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_ISR, 1);
				//xTaskCreatePinnedToCore(adc_mpu9250_fusionTask, "adc_mpu9250_fusionTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &FUSION_ISR, 1);

    
				if ( TIMER_start(samplePeriodHandle) != ESP_OK) return;
	            nextStatus = DONE;
	            break;
            }
			
            default:
                 break;
        }
    }
}

void printStatus(status_t nextStatus) {
    static status_t lastStatus = ERROR;

    if( lastStatus != nextStatus) {
        ESP_LOGI(TAG, "--------------%s-----------------\n",statusAsString[nextStatus]);
        lastStatus = nextStatus;
    }
}

void defineLogLevels() {
    esp_log_level_set(TAG,ESP_LOG_VERBOSE );
    esp_log_level_set(TAG_MQTT_RECIEVER, ESP_LOG_VERBOSE );
    esp_log_level_set(TAG_MQTT_PUBLISHER, ESP_LOG_VERBOSE );
    esp_log_level_set(TAG_SD, ESP_LOG_VERBOSE );
}

//----------------------------INTERRUPTIONS -----------------------------------

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    //DEBUG_PRINT_INTERRUPT_MAIN(">m>\n");
}

void IRAM_ATTR time_refreshInternalTimer(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    xTaskResumeFromISR(TIME_REFRESH_ISR);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT_MAIN(">t>\n");
}

void IRAM_ATTR adc_mpu9250_sampleSynchronize(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xTaskResumeFromISR(SAMPLE_LOG_ISR);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	xTaskResumeFromISR(SENSOR_ISR);
}

//------------------------------ TASKS -----------------------------------------

void IRAM_ATTR sampleSynchronizerTask (void *pvParameters ) {
	// This interruptions are created here to run on core 1.
	if ( TIMER_create("timer sample", TIMER_DEFAULT_SAMPLE_PERIOD_MS, adc_mpu9250_sampleSynchronize, &samplePeriodHandle) != ESP_OK) return;
	
	WDT_addTask(SAMPLE_LOG_ISR);
	struct timeval tv;
	while (true) {
		gettimeofday(&tv, NULL);
		DEBUG_PRINT_MAIN(TAG, "\n>>>>>>>>>>>>>>>>>>>>>>>>> ms: %ld", tv.tv_usec / 1000);
		WDT_reset(SAMPLE_LOG_ISR);
		vTaskSuspend(SAMPLE_LOG_ISR);
	}
}

void IRAM_ATTR sensors_readingTask(void *pvParameters) {
    ESP_LOGI(TAG_SENSORS, "MPU/ADC task init");
	// This interruptions are created here to run on core 1.
	if ( MPU9250_attachInterruptWith(mpu9250_enableReadingTaskByInterrupt, true) != ESP_OK) return;
	
	WDT_addTask(SENSOR_ISR);
    char timeMessage[TIME_MESSAGE_LENGTH];
    MPU9250_t mpuSample;
	ADC_t adcSample;
	timeInfo_t timeInfo;
	QueuePacket_t resultPacket;
	
	while (true) {
	    vTaskSuspend(SENSOR_ISR);
	    // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        if (xSemaphoreTake(xSemaphore_newDataOnMPU, 1) != pdTRUE) {
            ESP_LOGE(TAG_SENSORS, "MPU not ready");
            vTaskDelay(1 / portTICK_PERIOD_MS);
            continue;
        }

        TIME_asString(timeMessage);
        ESP_LOGI(TAG_SENSORS, "Task MPU9250/ADC Reading\t\t\t\t\t%s", timeMessage);

        TIME_getInfoTime(&timeInfo);
        if (MPU9250_ReadAcce(&mpuSample) != ESP_OK) {
            ESP_LOGE(TAG_SENSORS, "MPU Task Fail getting data");
            xSemaphoreGive(xSemaphore_newDataOnMPU);
            continue;
        }
		mpu9250_ready();
		
		if (ADC_GetRaw( &adcSample ) != ESP_OK) {
			ESP_LOGE(TAG_SENSORS, "ADC Task Fail getting data");
			xSemaphoreGive(xSemaphore_newDataOnMPU);
			continue;
		}

        TIME_asString(timeMessage);
        DEBUG_PRINT_MAIN(TAG_SENSORS, "Task MPU9250 Saving X:%d Y:%d Z:%d \t\t\t%s", mpuSample.accelX, mpuSample.accelY, mpuSample.accelZ,timeMessage);
        DEBUG_PRINT_MAIN(TAG_SENSORS, "Task ADC Saving     X:%d Y:%d Z:%d \t\t\t\t%s", adcSample.adcX, adcSample.adcY, adcSample.adcZ,timeMessage);
		
        TIME_asString(timeMessage);
        ESP_LOGI(TAG_SENSORS, "Task MPU9250/ADC Before semaphore SD \t\t\t\t%s", timeMessage);
        if ( xSemaphoreTake(xSemaphore_SDMutexQueue, 1) != pdTRUE) {
            ESP_LOGE(TAG_SENSORS, "Error taking SD mutex");
			xSemaphoreGive(xSemaphore_newDataOnMPU);
			continue;
		}
        TIME_asString(timeMessage);
        DEBUG_PRINT_MAIN(TAG_SENSORS, "Task SD sending >>>>>>>>>>> \t\t\t\t\t%s", timeMessage);
        if (!buildDataPacketForSD(mpuSample, adcSample, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,&resultPacket)) {
	        ESP_LOGE(TAG_SENSORS, "\t Error building packet ... losing samples");
        }

        if( xQueueSend(SDDataQueue, &resultPacket, portMAX_DELAY) != pdPASS ){
	        ESP_LOGE(TAG_SENSORS, "\t Error sending packet to SD queue");
        }
        xSemaphoreGive(xSemaphore_SDMutexQueue);
			
        TIME_asString(timeMessage);
        ESP_LOGI(TAG_SENSORS, "Task MPU9250/ADC Finish \t\t\t\t\t%s", timeMessage);
	    WDT_reset(SENSOR_ISR);
    }
}

void IRAM_ATTR sd_savingTask(void *pvParameters) {

    ESP_LOGI(TAG_SD,"SD task init");
    WDT_addTask(SD_ISR);

    timeInfo_t timeInfo;
    SD_time_t sdData;
    QueuePacket_t aReceivedPacket;
    SD_time_t sdDataArray[QUEUE_SAMPLE_LENGTH];
    char mainPathToSave[MAX_SAMPLE_PATH_LENGTH];
    char timeMessage[TIME_MESSAGE_LENGTH];

    DIR_getMainSampleDirectory(mainPathToSave);

    while (1) {
        int sampleCounterToSave = 0;
        if (takeSDQueueWhenSamplesAre(MIN_SAMPLES_TO_SAVE)) {
            TIME_asString(timeMessage);
            DEBUG_PRINT_MAIN(TAG_SD, "Task SD save init  >>>>>>>>>>> \t\t\t\t\t%s", timeMessage);
            while( xQueueReceive(SDDataQueue, &aReceivedPacket, 0)){
	            vTaskDelay(MIN_DELAY_FOR_WHILE );
	            sdData = getSDDataFromPacket(&aReceivedPacket);
                //DEBUG_PRINT_MAIN(TAG, "SD Task Received sdData");
                sample_change_case_t sampleTimeState = analyzeSampleTime(sdData.hour, sdData.min, sdData.seconds);
	            TIME_asString(timeMessage);
                DEBUG_PRINT_MAIN(TAG_SD, "SD Task Received sdData with state %s\t\t\t%s", printSampleTimeState(sampleTimeState),timeMessage);
                switch(sampleTimeState){
                    case NEW_MINUTE:
                        if(sampleCounterToSave != 0) {
                            SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
                            sampleCounterToSave = 0;
                        }
                        SD_setSampleFilePath(sdData.hour, sdData.min);
                        break;
                    case NEW_DAY:
                        // Si existe data guardada, implica que correspondía a antes de las 00:00:00
                        // entonces se guarda en el archivo de la fecha anterior.
                        if(sampleCounterToSave != 0) {
                            SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
                            sampleCounterToSave = 0;
                        }

                        TIME_getInfoTime(&timeInfo);
                        DIR_updateMainSampleDirectory(mainPathToSave, timeInfo.tm_year,timeInfo.tm_mon,timeInfo.tm_mday);
                        SD_setSampleFilePath(sdData.hour, sdData.min);
                        break;

                    case NO_CHANGE:
                    default: ;
                }
                sdDataArray[sampleCounterToSave] = sdData;
                sampleCounterToSave++;
            }
            TIME_asString(timeMessage);
            DEBUG_PRINT_MAIN(TAG_SD, "Task SD saving all data>>>>>>>>>>>\t\t\t\t\t%s", timeMessage);
            SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);

            TIME_asString(timeMessage);
            DEBUG_PRINT_MAIN(TAG_SD, "Task SD saved all data >>>>>>>>>>>\t\t\t\t\t%s", timeMessage);
            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
            xSemaphoreGive(xSemaphore_SDMutexQueue);

            TIME_asString(timeMessage);
            DEBUG_PRINT_MAIN(TAG_SD, "Task SD finish >>>>>>>>>>>\t\t\t\t\t\t%s", timeMessage);
        }
	    vTaskDelay(MIN_DELAY_FOR_WHILE );
	    WDT_reset(SD_ISR);
    }
}

void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters){
    ESP_LOGI(TAG_MQTT_RECIEVER,"MQTT receive command task init");
    WDT_addTask(MQTT_RECEIVE_ISR);
    char directoryPathToRetrieve[MAX_SAMPLE_PATH_LENGTH];

    while (1) {
        vTaskDelay(MIN_DELAY_FOR_WHILE);
        if (MQTT_HasCommandToProcess()){
            command_t command;
            char rawCommand[MAX_TOPIC_LENGTH];
            MQTT_GetCommand(rawCommand);
            if( COMMAND_Parse(rawCommand, &command) != ESP_OK){
                ESP_LOGE(TAG_MQTT_RECIEVER, "MQTT error parsing command %s", rawCommand);
                continue;
            }
            DEBUG_PRINT_MAIN(TAG_MQTT_RECIEVER, "MQTT Task Received command %s", COMMAND_GetHeaderType(command));
            switch ( command.header) {
                case RETRIEVE_DATA:{
                    int dayToGet = command.startDay;
                    int hourToGet = command.startHour;
                    int minuteToGet = command.startMinute;
                    DIR_updateRetrieveSampleDirectory(directoryPathToRetrieve, command.startYear, command.startMonth, dayToGet);

                    while(!COMMAND_matchEndTime(&command, dayToGet, hourToGet, minuteToGet)){
                        vTaskDelay(1);

                        SD_setRetrieveSampleFilePath(hourToGet, minuteToGet);
                        size_t totalDataRetrieved = 0;

                        SD_t *sdData = NULL;
                        if( SD_getDataFromRetrieveSampleFile(directoryPathToRetrieve, &sdData, &totalDataRetrieved) != ESP_OK){
                            MQTT_publish(TOPIC_TO_PUBLISH_DATA, "error reading", strlen("error reading"));
                            ESP_LOGE(TAG_MQTT_RECIEVER,"Error getting data from sample file");
                            break;
                        }

                        pushSDDataToMQTTQueueRoutine(totalDataRetrieved, sdData, minuteToGet, dayToGet);
                        free(sdData);

                        minuteToGet++;
                        if(minuteToGet == 60){
                            minuteToGet = 0;
                            hourToGet++;
                            if( hourToGet == 24){
                                hourToGet = 0;
                                dayToGet++;
                                DIR_updateRetrieveSampleDirectory(directoryPathToRetrieve, command.startYear, command.startMonth, dayToGet);
                            }
                        }
                    }
                    DEBUG_PRINT_MAIN(TAG_MQTT_RECIEVER,"MQTT Task Finished retrieving data");
                    break;
                }

            }

        }
        WDT_reset(MQTT_RECEIVE_ISR);
    }
}

void IRAM_ATTR mqtt_publishDataTask(void *pvParameters){
    ESP_LOGI(TAG_MQTT_PUBLISHER,"WIFI publish task init");
    WDT_addTask(MQTT_PUBLISH_ISR);
    SD_time_t item;
    char dataToSend[MAX_CHARS_PER_SAMPLE];
    char timeHeader[MAX_CHARS_PER_SAMPLE];
    char maxDataToSend[MAX_CHARS_PER_SAMPLE*100];

    int lastMinute = -1, lastHour = -1;
    while (1) {
        vTaskDelay(MIN_DELAY_FOR_WHILE);
        if(uxQueueMessagesWaiting(MQTTDataQueue) > 0 ){
            if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, 1) == pdTRUE) {
                MQTT_publish(TOPIC_TO_PUBLISH_DATA, "Start Sending", 13);
                while (uxQueueMessagesWaiting(MQTTDataQueue) != 0){
                    vTaskDelay(1);
                    if( xQueueReceive(MQTTDataQueue, &item, 0) != pdTRUE){
                        DEBUG_PRINT_MAIN(TAG_MQTT_PUBLISHER, "Error receiving data from queue");
                        break;
                    }

                    if (lastMinute != item.min || lastHour != item.hour){
                        if (strlen(maxDataToSend) > 0){
                            MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                            memset(maxDataToSend, 0, sizeof(maxDataToSend));
                        }
                        lastMinute = item.min;
                        lastHour = item.hour;
                        sprintf(timeHeader,"-----------%02d:%02d---------", lastHour, lastMinute);
                        MQTT_publish(TOPIC_TO_PUBLISH_DATA, timeHeader, strlen(timeHeader));
                    }

                    memset(dataToSend, 0, sizeof(dataToSend));
                    sprintf(dataToSend,"%04x%04x%04x%04x%04x%04x\n",
                            item.sensorsData.mpuData.accelX,
                            item.sensorsData.mpuData.accelY,
                            item.sensorsData.mpuData.accelZ,
                            item.sensorsData.adcData.adcX,
                            item.sensorsData.adcData.adcY,
                            item.sensorsData.adcData.adcZ);


                    if (strlen(maxDataToSend) + strlen(dataToSend) > MAX_CHARS_PER_SAMPLE*100){
                        MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                        memset(maxDataToSend, 0, sizeof(maxDataToSend));
                    } else{
                        strcat(maxDataToSend, dataToSend);
                    }
                    WDT_reset(MQTT_PUBLISH_ISR);
                }
                xSemaphoreGive(xSemaphore_MQTTMutexQueue);
                MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                memset(maxDataToSend, 0, sizeof(maxDataToSend));
                MQTT_publish(TOPIC_TO_PUBLISH_DATA, "Finish Sending", 14);
            }
        }
        WDT_reset(MQTT_PUBLISH_ISR);
    }
}

void IRAM_ATTR time_internalTimeSync(void *pvParameters){
    ESP_LOGI(TAG, "Time sync task init");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (1) {
	    vTaskDelay(MIN_DELAY_FOR_WHILE);
        vTaskSuspend(TIME_REFRESH_ISR);
	    TIME_synchronizeTimeAndDateFromInternet();
        TIME_printTimeNow();
    }
}


//------------------------------ UTILS -----------------------------------------

esp_err_t ESP32_initQueue() {
    MQTTDataQueue = xQueueCreate(QUEUE_MQTT_LENGTH, sizeof(SD_t));                  if(MQTTDataQueue == NULL)   return ESP_FAIL;
    SDDataQueue = xQueueCreate(QUEUE_SAMPLE_LENGTH, sizeof(QueuePacket_t));         if(SDDataQueue == NULL)     return ESP_FAIL;
    return ESP_OK;
}

esp_err_t ESP32_initSemaphores() {
    xSemaphore_newDataOnMPU = xSemaphoreCreateBinary(); if( xSemaphore_newDataOnMPU == NULL )   return ESP_FAIL;
	xSemaphore_MPU_ADCMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_MPU_ADCMutexQueue == NULL )  return ESP_FAIL;
    xSemaphore_MQTTMutexQueue = xSemaphoreCreateMutex();if( xSemaphore_MQTTMutexQueue == NULL ) return ESP_FAIL;
    xSemaphore_SDMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_SDMutexQueue == NULL )  return ESP_FAIL;
    return ESP_OK;
}

char * printSampleTimeState(sample_change_case_t state) {
    switch (state) {
        case NEW_MINUTE: return "NEW_MINUTE";
        case NEW_DAY: return "NEW_DAY";
        case NO_CHANGE: return "NO_CHANGE";
        default:
            ESP_LOGE(TAG, "UNKNOWN STATE for sample");
            return "UNKNOWN";
    }
}


bool takeSDQueueWhenSamplesAre(int numberOfSamples) {
	char timeMessage[TIME_MESSAGE_LENGTH];
	UBaseType_t lenOfSDQueue = uxQueueMessagesWaiting(SDDataQueue);
    if (lenOfSDQueue >= numberOfSamples) {
        if (xSemaphoreTake(xSemaphore_SDMutexQueue, 1) == pdTRUE) {
	        TIME_asString(timeMessage);
	        DEBUG_PRINT_MAIN(TAG_SD, "Task SD Taked  >>>>>>>>>>>\t\t\t\t\t\t%s", timeMessage);
	        return true;
        }
    }
    return false;
}

sample_change_case_t analyzeSampleTime(int hour, int min, int seconds) {
    sample_change_case_t changeCase = NO_CHANGE;
    static int lastMinute = -1;
    static bool notifyNewDay = false;
    if (lastMinute != min){
        lastMinute = min;
        changeCase = NEW_MINUTE;
    }
    if (hour == 0 && min == 0 && seconds == 0){
        if( !notifyNewDay ){
            notifyNewDay = true;
            changeCase = NEW_DAY;
        }
    } else {
        notifyNewDay = false;
    }

    return changeCase;
}

void pushSDDataToMQTTQueueRoutine(size_t totalDataRetrieved, const SD_t *sdData, int minute, int hour) {
    int sampleSent = 0;
    while(sampleSent != totalDataRetrieved){
        if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, 5) == pdTRUE){
            while(sampleSent < totalDataRetrieved){
                vTaskDelay(1);
                SD_time_t aSDdataToSend;
                memcpy(&aSDdataToSend.sensorsData, &sdData[sampleSent], sizeof(SD_t));
                aSDdataToSend.hour = hour;
                aSDdataToSend.min = minute;
                if ( xQueueSend(MQTTDataQueue, &aSDdataToSend, 0) == errQUEUE_FULL){
                    ESP_LOGE(TAG,"Error sending data to MQTT queue on sample %d", sampleSent);
                    break;
                }
                sampleSent++;
                WDT_reset(MQTT_RECEIVE_ISR);
            }
            xSemaphoreGive(xSemaphore_MQTTMutexQueue);
        }
    }
}

