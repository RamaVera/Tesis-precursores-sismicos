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
#define MIN_SAMPLES_TO_SAVE 200
#define QUEUE_SAMPLE_LENGTH (MIN_SAMPLES_TO_SAVE + 1)
#define QUEUE_MQTT_LENGTH 2000
#define MIN_DELAY_FOR_WHILE (1)
#define MAX_LOG_MESSAGES 30

TaskHandle_t TIMER_CREATOR_ISR = NULL;
TaskHandle_t SD_ISR = NULL;
TaskHandle_t LOG_ISR = NULL;
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


typedef struct logMessages_t{
	char tag[20];
	timeval_t time;
	char message[100];
} logMessages_t;


static timeval_t programStartTime;
logMessages_t logMessages[MAX_LOG_MESSAGES];
int logMessagesIndex = 0;

//#define DEBUG
#ifdef DEBUG
#define SAMPLE_PERIOD_MS SAMPLE_PERIOD_FOR_LOGS_MS
#define DEBUG_PRINT_MAIN(tag, message) appendMessageWithTime(tag, message);
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define SAMPLE_PERIOD_MS DEFAULT_SAMPLE_PERIOD_MS
#define DEBUG_PRINT_MAIN(tag, message) do {} while (0)
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) do {} while (0)
#endif

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);
void IRAM_ATTR time_refreshInternalTimer(void* pvParameters);
void IRAM_ATTR adc_mpu9250_sampleSynchronize(void* pvParameters);

void IRAM_ATTR timer_creatorTask (void *pvParameters );
void IRAM_ATTR sensors_readingTask(void *pvParameters);

void IRAM_ATTR sd_savingTask(void *pvParameters);
void IRAM_ATTR mqtt_publishDataTask(void *pvParameters);
void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters);
void IRAM_ATTR time_internalTimeSync(void *pvParameters);
void IRAM_ATTR log_Task(void *pvParameters);

bool takeSDQueueWhenSamplesAre(int numberOfSamples);
sample_change_case_t analyzeSampleTime(int hour, int min, int seconds);
char * printSampleTimeState(sample_change_case_t state);
void appendMessageWithTime ( const char *tag, char *message );
char* toString(const char* format, ...);
int enqueueDataForMQTTSend ( int hourToGet, int minuteToGet, const SD_t *sdData, size_t sdDataLength );

void app_main (void ) {

    status_t nextStatus = INIT_MODULES;
    config_params_t params;
    wifiParams_t wifiParams;
    mqttParams_t mqttParams;
    timeInfo_t timeInfo;

    defineLogLevels();
	ESP_LOGI(TAG, "Run Main on Core ID: %d", xPortGetCoreID()); // Main Runs on Core 0
	TIME_saveSnapshot( &programStartTime );
	
    while(nextStatus != DONE){
        printStatus(nextStatus);

        switch (nextStatus) {
	        case INIT_MODULES: {
		        if ( Button_init() != ESP_OK) return;
		        if ( ADC_Init() != ESP_OK) return;
		        if ( MPU9250_init() != ESP_OK) return;
		        if ( WDT_init(WATCHDOG_TIMEOUT_IN_SECONDS) != ESP_OK) return;
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
					nextStatus = ERROR;
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
	            gpio_pad_select_gpio(GPIO_NUM_32);
	            gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
	            gpio_pad_select_gpio(GPIO_NUM_15);
	            gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
	            TIME_printTimeAndDate(&timeInfo);
	            if (DIR_setMainSampleDirectory(timeInfo.tm_year, timeInfo.tm_mon, timeInfo.tm_mday) != ESP_OK) return;
	
	            // CORE 0
	            xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 6, &SD_ISR,0);

	            // CORE 1
	            xTaskCreatePinnedToCore( timer_creatorTask, "sample_synchronizerTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 4, &TIMER_CREATOR_ISR, 1);
	            xTaskCreatePinnedToCore(sensors_readingTask, "sensors_readingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 3, &SENSOR_ISR, 1);
#ifdef DEBUG
	            xTaskCreatePinnedToCore(log_Task, "log_Task", 1024 * 16, NULL, tskIDLE_PRIORITY + 1, &LOG_ISR,1);
#endif
    
				if ( TIMER_start(samplePeriodHandle) != ESP_OK) return;
	            nextStatus = DONE;
	            break;
            }
			
			case ERROR: {
		        esp_restart();
	        };
			
            default:
                 break;
        }
    }
}

//----------------------------INTERRUPTIONS -----------------------------------

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IRAM_ATTR time_refreshInternalTimer(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    xTaskResumeFromISR(TIME_REFRESH_ISR);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT_MAIN(">t>\n");
}

void IRAM_ATTR adc_mpu9250_sampleSynchronize(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xTaskResumeFromISR(SENSOR_ISR);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#ifdef DEBUG
	xTaskResumeFromISR(LOG_ISR);
#endif
}

//------------------------------ TASKS -----------------------------------------

void IRAM_ATTR log_Task(void *pvParameters) {
	ESP_LOGI(TAG, "Log task init on Core ID: %d", xPortGetCoreID());
	timeInfo_t toPrint;
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	
	WDT_addTask(LOG_ISR);
	while (true) {
		for (int i = 0; i < logMessagesIndex; i++){
			TIME_Diff( &toPrint, &programStartTime, &(logMessages[i].time));
			ESP_LOGI(TAG, "Time: %02d:%02d:%02d.%03d.%03d -- %02d:%s", toPrint.tm_hour,toPrint.tm_min, toPrint.tm_sec , toPrint.milliseconds ,toPrint.microseconds,i, logMessages[i].message);
			vTaskDelay(MIN_DELAY_FOR_WHILE);
		}
		logMessagesIndex = 0;
		WDT_reset(LOG_ISR);
		vTaskSuspend(LOG_ISR);
	}
}

void IRAM_ATTR timer_creatorTask (void *pvParameters ) {
	// This interruptions are created here to run on core 1.
	if ( TIMER_create( "timer sample", SAMPLE_PERIOD_MS, adc_mpu9250_sampleSynchronize, &samplePeriodHandle) != ESP_OK) return;
	vTaskSuspend( TIMER_CREATOR_ISR);
}

void IRAM_ATTR sensors_readingTask(void *pvParameters) {
	ESP_LOGI(TAG_SENSORS, "MPU/ADC task init on Core ID: %d", xPortGetCoreID());
	
	// This interruptions are created here to run on core 1.
	if ( MPU9250_attachInterruptWith(mpu9250_enableReadingTaskByInterrupt, true) != ESP_OK) return;
	
    MPU9250_t mpuSample;
	ADC_t adcSample;
	timeInfo_t timeInfo;
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	WDT_addTask(SENSOR_ISR);
	
	gpio_set_level(GPIO_NUM_32, 0);
	
	while (true) {
		
		vTaskSuspend(SENSOR_ISR);
		gpio_set_level(GPIO_NUM_32, 1);
		DEBUG_PRINT_MAIN( TAG_SENSORS, "Task MPU9250/ADC Start" );
		
		// xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        if (xSemaphoreTake(xSemaphore_newDataOnMPU, 1) != pdTRUE) {
            ESP_LOGE(TAG_SENSORS, "MPU not ready");
            vTaskDelay(1 / portTICK_PERIOD_MS);
            continue;
        }
		//DEBUG_PRINT_MAIN( TAG_SENSORS, "MPU9250 Reading" );

        TIME_getInfoTime(&timeInfo);
        if (MPU9250_ReadAcce(&mpuSample) != ESP_OK) {
            ESP_LOGE(TAG_SENSORS, "MPU Task Fail getting data");
            xSemaphoreGive(xSemaphore_newDataOnMPU);
            continue;
        }
		mpu9250_ready();
		
		//DEBUG_PRINT_MAIN( TAG_SENSORS, "ADC Reading" );
		if (ADC_GetRaw( &adcSample ) != ESP_OK) {
			ESP_LOGE(TAG_SENSORS, "ADC Task Fail getting data");
			xSemaphoreGive(xSemaphore_newDataOnMPU);
			continue;
		}
		
		//DEBUG_PRINT_MAIN( TAG_SENSORS, "Task MPU9250/ADC Before semaphore SD" );
		if ( xSemaphoreTake(xSemaphore_SDMutexQueue, 1) != pdTRUE) {
            ESP_LOGE(TAG_SENSORS, "Error taking SD mutex");
			xSemaphoreGive(xSemaphore_newDataOnMPU);
			continue;
		}
		
		//DEBUG_PRINT_MAIN( TAG_SENSORS, "Task SD sending" );
		SD_time_t SDdata;
		SDdata.sensorsData.adcData.adcX = adcSample.adcX;
		SDdata.sensorsData.adcData.adcY = adcSample.adcY;
		SDdata.sensorsData.adcData.adcZ = adcSample.adcZ;
		SDdata.sensorsData.mpuData.accelX = mpuSample.accelX;
		SDdata.sensorsData.mpuData.accelY = mpuSample.accelY;
		SDdata.sensorsData.mpuData.accelZ = mpuSample.accelZ;
		SDdata.hour = timeInfo.tm_hour;
		SDdata.min = timeInfo.tm_min;
		SDdata.seconds = timeInfo.tm_sec;

        if( xQueueSend(SDDataQueue, &SDdata, portMAX_DELAY) != pdPASS ){
	        ESP_LOGE(TAG_SENSORS, "\t Error sending packet to SD queue");
        }
        xSemaphoreGive(xSemaphore_SDMutexQueue);
		
		DEBUG_PRINT_MAIN( TAG_SENSORS, "Task MPU9250/ADC Finish" );
		gpio_set_level(GPIO_NUM_32, 0);
		WDT_reset(SENSOR_ISR);
    }
}

void IRAM_ATTR sd_savingTask(void *pvParameters) {
	gpio_set_level(GPIO_NUM_15, 0);

    ESP_LOGI(TAG_SD,"SD task init on Core ID: %d", xPortGetCoreID());
	
	char mainPathToSave[MAX_SAMPLE_PATH_LENGTH];
#ifdef DEBUG
	int counter = 0;
#endif
    timeInfo_t timeInfo;
    SD_time_t sdData;
    SD_time_t * sdDataArray = (SD_time_t *) malloc(QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
	if (sdDataArray == NULL) {
		ESP_LOGE(TAG_SD, "Error allocating memory for sdDataArray");
		return;
	}

    DIR_getMainSampleDirectory(mainPathToSave);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	
	WDT_addTask(SD_ISR);
	while (1) {
	    int sampleCounterToSave = 0;
        if (takeSDQueueWhenSamplesAre(MIN_SAMPLES_TO_SAVE)) {
	        gpio_set_level(GPIO_NUM_15, 1);
	        DEBUG_PRINT_MAIN( TAG_SD, "Task SD save init" );
			while( xQueueReceive(SDDataQueue, &sdData, 0)){
                sample_change_case_t sampleTimeState = analyzeSampleTime(sdData.hour, sdData.min, sdData.seconds);
#ifdef DEBUG
				//DEBUG_PRINT_MAIN( TAG_SD, toString("SD Task Received sdData with state %s", printSampleTimeState(sampleTimeState)) );
				if (++counter % 10 == 0) DEBUG_PRINT_MAIN( TAG_SD, toString("%d,%d,%d-%d,%d,%d-%02d:%02d:%02d",sdData.sensorsData.mpuData.accelX,sdData.sensorsData.mpuData.accelY,sdData.sensorsData.mpuData.accelZ,sdData.sensorsData.adcData.adcX,sdData.sensorsData.adcData.adcY,sdData.sensorsData.adcData.adcZ,sdData.hour,sdData.min,sdData.seconds));
#endif
				switch(sampleTimeState){
                    case NEW_MINUTE:
						if(sampleCounterToSave != 0) {
							SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
                            sampleCounterToSave = 0;
							DEBUG_PRINT_MAIN(TAG_SD, toString("Sample time last: %02d:%02d:%02d", sdDataArray[sampleCounterToSave-1].hour, sdDataArray[sampleCounterToSave-1].min, sdDataArray[sampleCounterToSave-1].seconds));
						}
                        SD_setSampleFilePath(sdData.hour, sdData.min);
						DEBUG_PRINT_MAIN(TAG_SD, toString("Sample time next: %02d:%02d:%02d", sdData.hour, sdData.min, sdData.seconds));
						break;
                    case NEW_DAY:
						// Si existe data guardada, implica que correspondía a antes de las 00:00:00
                        // entonces se guarda en el archivo de la fecha anterior.
                        if(sampleCounterToSave != 0) {
	                        SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
                            sampleCounterToSave = 0;
	                        DEBUG_PRINT_MAIN(TAG_SD, toString("Sample time last: %02d:%02d:%02d", sdDataArray[sampleCounterToSave-1].hour, sdDataArray[sampleCounterToSave-1].min, sdDataArray[sampleCounterToSave-1].seconds));
                        }
                        TIME_getInfoTime(&timeInfo);
                        DIR_updateMainSampleDirectory(mainPathToSave, timeInfo.tm_year,timeInfo.tm_mon,timeInfo.tm_mday);
                        SD_setSampleFilePath(sdData.hour, sdData.min);
						DEBUG_PRINT_MAIN(TAG_SD, toString("Sample time: %02d:%02d:%02d", sdData.hour, sdData.min, sdData.seconds));
						break;

                    case NO_CHANGE:
                    default: ;
                }
                sdDataArray[sampleCounterToSave] = sdData;
                sampleCounterToSave++;
            }
	        xSemaphoreGive(xSemaphore_SDMutexQueue);
	        gpio_set_level(GPIO_NUM_15, 0);
	        gpio_set_level(GPIO_NUM_15, 1);
			
	        SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
            memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_time_t));
	        DEBUG_PRINT_MAIN( TAG_SD, "Task SD finish" );
        }
	    vTaskDelay(MIN_DELAY_FOR_WHILE );
	    WDT_reset(SD_ISR);
	    gpio_set_level(GPIO_NUM_15, 0);
    }
}

void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters){
    ESP_LOGI(TAG_MQTT_RECIEVER,"MQTT receive command task init on Core ID: %d", xPortGetCoreID());
	
	char directoryPathToRetrieve[MAX_SAMPLE_PATH_LENGTH];
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	
	WDT_addTask(MQTT_RECEIVE_ISR);
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
            //DEBUG_PRINT_MAIN(TAG_MQTT_RECIEVER, toString("MQTT Task Received command %s", COMMAND_GetHeaderType(command)));
            switch ( command.header) {
                case RETRIEVE_DATA:{
                    int dayToGet = command.startDay;
                    int hourToGet = command.startHour;
                    int minuteToGet = command.startMinute;
                    DIR_updateRetrieveSampleDirectory(directoryPathToRetrieve, command.startYear, command.startMonth, dayToGet);

                    while(!COMMAND_matchEndTime(&command, dayToGet, hourToGet, minuteToGet)){
                        vTaskDelay(MIN_DELAY_FOR_WHILE);

                        SD_setRetrieveSampleFilePath(hourToGet, minuteToGet);
						int endOfFile = 0;
	                    SD_t *sdData = NULL;
	                    size_t sizeRetrieved = 0;
						long line = 0;
	
	                    while ( !endOfFile){
							vTaskDelay(MIN_DELAY_FOR_WHILE);
							if( SD_readDataFromRetrieveSampleFile( directoryPathToRetrieve, &sdData, &sizeRetrieved, &line, &endOfFile) != ESP_OK){
								MQTT_publish(TOPIC_TO_PUBLISH_DATA, "error reading", strlen("error reading"));
								ESP_LOGE(TAG_MQTT_RECIEVER,"Error getting data from sample file");
								break;
							}
							if (sdData != NULL && sizeRetrieved > 0){
								int sampleSent = enqueueDataForMQTTSend( hourToGet, minuteToGet, sdData, sizeRetrieved );
								free(sdData);
								sdData = NULL;
								if (sampleSent != sizeRetrieved) {
									ESP_LOGE(TAG_MQTT_RECIEVER, "Error sending data to MQTT queue");
									break;
								}
								ESP_LOGI(TAG_MQTT_RECIEVER, "MQTT Sent %d data",sampleSent);
							}
						}
						
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
    ESP_LOGI(TAG_MQTT_PUBLISHER,"WIFI publish task init on Core ID: %d", xPortGetCoreID());
	
    SD_time_t item;
    char dataToSend[MAX_CHARS_PER_SAMPLE];
    char timeHeader[MAX_CHARS_PER_SAMPLE];
	
    char * maxDataToSend = (char *) malloc(sizeof(char) * MAX_DATA_FOR_MQTT_TRANSFER);
	memset(maxDataToSend, 0, sizeof(MAX_DATA_FOR_MQTT_TRANSFER));

    int lastMinute = -1, lastHour = -1;
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	
	WDT_addTask(MQTT_PUBLISH_ISR);
    while (1) {
        vTaskDelay(MIN_DELAY_FOR_WHILE);
        if(uxQueueMessagesWaiting(MQTTDataQueue) > 0 ){
            if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, 1) == pdTRUE) {
                while (uxQueueMessagesWaiting(MQTTDataQueue) != 0){
                    vTaskDelay(MIN_DELAY_FOR_WHILE);
                    if( xQueueReceive(MQTTDataQueue, &item, 0) != pdTRUE){
                        DEBUG_PRINT_MAIN(TAG_MQTT_PUBLISHER, "Error receiving data from queue");
                        break;
                    }

                    if (lastMinute != item.min || lastHour != item.hour){
                        if (strlen(maxDataToSend) > 0){
                            MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                            memset(maxDataToSend, 0, sizeof(MAX_DATA_FOR_MQTT_TRANSFER));
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
					
                    if (strlen(maxDataToSend) + strlen(dataToSend) > MAX_DATA_FOR_MQTT_TRANSFER){
                        MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                        memset(maxDataToSend, 0, sizeof(MAX_DATA_FOR_MQTT_TRANSFER));
                    }
					strcat(maxDataToSend, dataToSend);
                    WDT_reset(MQTT_PUBLISH_ISR);
                }
                xSemaphoreGive(xSemaphore_MQTTMutexQueue);
                MQTT_publish(TOPIC_TO_PUBLISH_DATA, maxDataToSend, strlen(maxDataToSend));
                memset(maxDataToSend, 0, sizeof(MAX_DATA_FOR_MQTT_TRANSFER));
            }
        }
        WDT_reset(MQTT_PUBLISH_ISR);
    }
}

void IRAM_ATTR time_internalTimeSync(void *pvParameters){
    ESP_LOGI(TAG, "Time sync task init on Core ID: %d", xPortGetCoreID());
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
	MQTTDataQueue = xQueueCreate(QUEUE_MQTT_LENGTH, sizeof(SD_time_t)); if(MQTTDataQueue == NULL)   return ESP_FAIL;
	SDDataQueue = xQueueCreate(QUEUE_SAMPLE_LENGTH, sizeof(SD_time_t)); if(SDDataQueue == NULL)     return ESP_FAIL;
	return ESP_OK;
}

esp_err_t ESP32_initSemaphores() {
	xSemaphore_newDataOnMPU = xSemaphoreCreateBinary(); if( xSemaphore_newDataOnMPU == NULL )   return ESP_FAIL;
	xSemaphore_MPU_ADCMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_MPU_ADCMutexQueue == NULL )  return ESP_FAIL;
	xSemaphore_MQTTMutexQueue = xSemaphoreCreateMutex();if( xSemaphore_MQTTMutexQueue == NULL ) return ESP_FAIL;
	xSemaphore_SDMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_SDMutexQueue == NULL )  return ESP_FAIL;
	return ESP_OK;
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
	UBaseType_t lenOfSDQueue = uxQueueMessagesWaiting(SDDataQueue);
    if (lenOfSDQueue >= numberOfSamples) {
        if (xSemaphoreTake(xSemaphore_SDMutexQueue, 1) == pdTRUE) {
	        DEBUG_PRINT_MAIN(TAG_SD, "Task SD Taked");
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

int enqueueDataForMQTTSend ( int hourToGet, int minuteToGet, const SD_t *sdData, size_t sdDataLength ) {
	int sampleSent = 0;
	while ( sampleSent != sdDataLength ) {
		vTaskDelay( MIN_DELAY_FOR_WHILE);
		if ( xSemaphoreTake( xSemaphore_MQTTMutexQueue, 1 ) == pdTRUE) {
			SD_time_t aSDdataToSend;
			memcpy( &aSDdataToSend.sensorsData, &sdData[ sampleSent ], sizeof( SD_t ));
			aSDdataToSend.hour = hourToGet;
			aSDdataToSend.min = minuteToGet;
			if ( xQueueSend( MQTTDataQueue, &aSDdataToSend, 0 ) == errQUEUE_FULL) {
				ESP_LOGE( TAG, "Error sending data to MQTT queue on sample %d",sampleSent );
				return sampleSent;
			}
			sampleSent++;
			xSemaphoreGive( xSemaphore_MQTTMutexQueue );
		}
		WDT_reset( MQTT_RECEIVE_ISR );
	}
	return sampleSent;
}


void appendMessageWithTime ( const char *tag, char *message ) {
	timeval_t timeSnapshot;
	TIME_saveSnapshot(&timeSnapshot);
	if (logMessagesIndex > MAX_LOG_MESSAGES){
		ESP_LOGW(TAG, "Log messages index overflow");
		logMessagesIndex = 0;
	}
	logMessages[logMessagesIndex].time = timeSnapshot;
	strcpy(logMessages[logMessagesIndex].message, message);
	strcpy(logMessages[logMessagesIndex].tag, tag);
	logMessagesIndex++;
}

char* toString(const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	// Calculamos el tamaño necesario para la cadena de salida
	int size = vsnprintf(NULL, 0, format, args);
	
	// Reseteamos la lista de argumentos
	va_end(args);
	va_start(args, format);
	
	// Reservamos memoria para la cadena de salida
	char* output = malloc(size + 1); // +1 para el caracter nulo '\0'
	if (output == NULL) {
		return NULL; // Si no se pudo reservar memoria, retornamos NULL
	}
	
	// Generamos la cadena de salida
	vsprintf(output, format, args);
	
	va_end(args);
	
	return output;
}
