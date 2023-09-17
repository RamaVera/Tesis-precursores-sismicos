/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include "main.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT_MAIN(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_MAIN(tag, fmt, ...) do {} while (0)
#define DEBUG_PRINT_INTERRUPT_MAIN(fmt, ...) do {} while (0)
#endif

/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_SAMPLE_LENGTH 50
#define QUEUE_SAMPLE_ITEM_SIZE sizeof(QueuePacket_t)
#define QUEUE_MQTT_LENGTH 2000
#define QUEUE_MQTT_SAMPLE_ITEM_SIZE sizeof(SD_sensors_data_t)

TaskHandle_t FUSION_ISR = NULL;
TaskHandle_t MPU_ISR = NULL;
TaskHandle_t MPU_CALIB_ISR = NULL;
TaskHandle_t SD_ISR = NULL;
TaskHandle_t ADC_ISR = NULL;
TaskHandle_t MQTT_PUBLISH_ISR = NULL;
TaskHandle_t MQTT_RECEIVE_ISR = NULL;
TaskHandle_t TIME_ISR = NULL;

QueueHandle_t SDDataQueue;
QueueHandle_t ADCDataQueue;
QueueHandle_t MPUDataQueue;
QueueHandle_t MQTTDataQueue;

SemaphoreHandle_t xSemaphore_newDataOnMPU = NULL;
SemaphoreHandle_t xSemaphore_MPUMutexQueue = NULL;
SemaphoreHandle_t xSemaphore_ADCMutexQueue = NULL;
SemaphoreHandle_t xSemaphore_MQTTMutexQueue = NULL;
SemaphoreHandle_t xSemaphore_SDMutexQueue = NULL;

bool calibrationDone = false;

uint64_t sampleRateInMS = 10 * portTICK_PERIOD_MS;

static const char *TAG = "MAIN "; // Para los mensajes del micro

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);
void IRAM_ATTR time_refreshInternalTimer(void* pvParameters);

void IRAM_ATTR adc_readingTask(void *pvParameters);
void IRAM_ATTR mpu9250_readingTask(void *pvParameters);
void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters);
void IRAM_ATTR adc_mpu9250_fusionTask(void *pvParameter);
void IRAM_ATTR sd_savingTask(void *pvParameters);
void IRAM_ATTR mqtt_publishDataTask(void *pvParameters);
void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters);
void IRAM_ATTR time_internalTimeSync(void *pvParameters);


bool takeSensorQueueWhenSamplesAre(int numberOfSamples);
QueuePacket_t * copyQueueToBuffer(QueueHandle_t queue, size_t * bufferLength);
void giveAllSensorSemaphores();
sample_change_case_t analyzeSampleTime(int hour, int min, int seconds);
char * printSampleTimeState(sample_change_case_t state);
void matchSensorsSamples(QueuePacket_t *mpuBuffer, QueuePacket_t *adcBuffer, size_t mpuLength, size_t adcLength);
TickType_t getTimeDiffInMs(TickType_t initTime);
void pushSDDataToMQTTQueueRoutine(size_t totalDataRetrieved, const SD_sensors_data_t *sdData, int minute, int hour);

void app_main(void) {

    status_t nextStatus = INIT_CONFIG;

    defineLogLevels();
    while(nextStatus != DONE){
        printStatus(nextStatus);

        switch (nextStatus) {
            case INIT_CONFIG:{
                config_params_t params;
                wifiParams_t wifiParams;
                mqttParams_t mqttParams;
                timeInfo_t timeInfo;

                if (SD_init() != ESP_OK) return;
                if (SD_getConfigurationParams(&params) != ESP_OK) return;

                if ( WIFI_parseParams(params.wifi_ssid, params.wifi_password, &wifiParams) != ESP_OK) return;
                if ( MQTT_parseParams(params.mqtt_ip_broker,params.mqtt_port,params.mqtt_user,params.mqtt_password,&mqttParams) != ESP_OK) return;
                if ( TIME_parseParams(params.init_year, params.init_month, params.init_day, &timeInfo) != ESP_OK) return;

                if ( WIFI_init(wifiParams) == ESP_OK) {
                    if ( WIFI_connect() == ESP_OK ) {
                        if ( MQTT_init(mqttParams) != ESP_OK) return;
                        MQTT_subscribe(TOPIC_TO_RECEIVE_COMMANDS);
                        if ( TIME_synchronizeTimeAndDate() != ESP_OK) break;
                        if (TIMER_create(time_refreshInternalTimer) != ESP_OK) return;

                        TIME_getInfoTime(&timeInfo);
                        TIME_updateParams(timeInfo,params.init_year, params.init_month, params.init_day);
                        SD_saveLastConfigParams(&params);
                        TIMER_start();
                    }
                }
                TIME_printTimeAndDate(&timeInfo);

                if ( DIR_setMainSampleDirectory( timeInfo.tm_year,timeInfo.tm_mon,timeInfo.tm_mday) != ESP_OK) return;
                nextStatus = INIT_SENSORS;
                break;
            }
            case INIT_SENSORS: {
                if ( Button_init() != ESP_OK) return;
                if ( ADC_Init() != ESP_OK) return;
                if ( MPU9250_init() != ESP_OK) return;
                if ( MPU9250_attachInterruptWith(mpu9250_enableReadingTaskByInterrupt, false) != ESP_OK) return;
                if ( ESP32_initSemaphores() != ESP_OK) return;
                if ( ESP32_initQueue() != ESP_OK) return;
                nextStatus = WAITING_TO_INIT;
                break;
            }
            case WAITING_TO_INIT: {
                static bool firstTime = true;
                if (firstTime) {
                    WDT_disableOnAllCores();
                    firstTime = false;
                }
                if (Button_isPressed()) {
                    WDT_enableOnAllCores();
                    nextStatus = INIT_SAMPLING;
                }
                break;
            }
//            case CALIBRATION: {
//                static bool creatingTask = true;
//                if (creatingTask) {
//                    creatingTask = false;
//                    xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &MPU_ISR, 0);
//                    xTaskCreatePinnedToCore(mpu9250_calibrationTask, "mpu9250_calibrationTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_CALIB_ISR, 1);
//                    MPU9250_enableInterrupt(true);
//                }
//
//                static bool firstTimeForCalibration = true;
//                if( calibrationDone && firstTimeForCalibration) {
//                    firstTimeForCalibration = false;
//                    MPU9250_enableInterrupt(false);
//                    vTaskDelay(100/portTICK_PERIOD_MS);
//                    WDT_removeTask(MPU_ISR);
//                    WDT_removeTask(MPU_CALIB_ISR);
//                    vTaskDelete(MPU_ISR);
//                    vTaskDelete(MPU_CALIB_ISR);
//
//                    ESP_LOGI(TAG,"---------------Finish Calibration--------------");
//                    nextStatus = INIT_SAMPLING;
//                }
//                break;
//            }
            case INIT_SAMPLING: {
                // CORE 1
                xTaskCreatePinnedToCore(adc_readingTask, "adc_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &ADC_ISR, 1);
                xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_ISR, 1);
                xTaskCreatePinnedToCore(adc_mpu9250_fusionTask, "adc_mpu9250_fusionTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, &FUSION_ISR,1);

                // CORE 0
                xTaskCreatePinnedToCore(mqtt_receiveCommandTask, "mqtt_receiveCommandTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 6, &MQTT_RECEIVE_ISR, 0);
                xTaskCreatePinnedToCore(time_internalTimeSync, "time_internalTimeSync", 1024 * 16, NULL,tskIDLE_PRIORITY + 5, &TIME_ISR, 0);
                xTaskCreatePinnedToCore(mqtt_publishDataTask, "mqtt_publishDataTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MQTT_PUBLISH_ISR, 0);
                xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, &SD_ISR,0);
                MPU9250_enableInterrupt(true);
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
    esp_log_level_set("MAIN", ESP_LOG_VERBOSE );
    esp_log_level_set("MPU9250", ESP_LOG_INFO );
    esp_log_level_set("SD_CARD", ESP_LOG_INFO );
    esp_log_level_set("WIFI", ESP_LOG_ERROR );
    esp_log_level_set("MQTT", ESP_LOG_INFO );
}

//----------------------------INTERRUPTIONS -----------------------------------

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT_MAIN(">>>\n");
}

void IRAM_ATTR time_refreshInternalTimer(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    xTaskResumeFromISR(TIME_ISR);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT_MAIN("<<<\n");
}

//------------------------------ TASKS -----------------------------------------

void IRAM_ATTR mpu9250_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "MPU task init");
    WDT_addTask(MPU_ISR);
    vTaskDelay(1 / portTICK_PERIOD_MS);

    while (1) {
        // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        if ( xSemaphoreTake(xSemaphore_newDataOnMPU, 1) != pdTRUE ){
            vTaskDelay(1 / portTICK_PERIOD_MS);
            continue;
        }
        TickType_t initTime = xTaskGetTickCount();
        MPU9250_t data;
        if( MPU9250_ReadAcce(&data)  != ESP_OK){
            ESP_LOGE(TAG, "MPU Task Fail getting data");
            xSemaphoreGive(xSemaphore_newDataOnMPU);
            continue;
        }
        ESP_LOGI(TAG, "Task MPU9250 Reading >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));

        // xSemaphore_queue toma el semaforo para que se acceda a la cola
        if(xSemaphoreTake(xSemaphore_MPUMutexQueue, portMAX_DELAY) == pdTRUE){
            QueuePacket_t aPacket;
            if( !buildDataPacketForMPU(data, &aPacket)){
                ESP_LOGE(TAG,"Error building packet");
                xSemaphoreGive(xSemaphore_newDataOnMPU);
                continue;
            }
            xQueueSend(MPUDataQueue, &aPacket, portMAX_DELAY);
            xSemaphoreGive(xSemaphore_MPUMutexQueue);
        }
        ESP_LOGI(TAG, "Task MPU9250 Saving >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));

        // Avisa que el dato fue leido por lo que baja el pin de interrupcion esperando que se haga un nuevo
        // flanco ascendente
        mpu9250_ready();
        WDT_reset(MPU_ISR);

        vTaskDelayUntil(&initTime, pdMS_TO_TICKS(sampleRateInMS));
    }
}

void IRAM_ATTR adc_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "ADC task init");
    WDT_addTask(ADC_ISR);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    while (1) {
        TickType_t initTime = xTaskGetTickCount();
        ADC_t data = ADC_GetRaw();
        DEBUG_PRINT_MAIN(TAG, "Task ADC Reading sample X:%d Y:%d Z:%d >> \t %u ms",data.adcX,data.adcY,data.adcZ, getTimeDiffInMs(initTime));

        if(xSemaphoreTake(xSemaphore_ADCMutexQueue, portMAX_DELAY) == pdTRUE){
            QueuePacket_t aPacket;
            if( !buildDataPacketForADC(data.adcY, data.adcX, data.adcZ, &aPacket)){
                ESP_LOGE(TAG,"Error building packet");
                continue;
            }
            DEBUG_PRINT_MAIN(TAG, "Task ADC building packet >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));
            xQueueSend(ADCDataQueue, &aPacket, portMAX_DELAY);
            xSemaphoreGive(xSemaphore_ADCMutexQueue);
        }
        DEBUG_PRINT_MAIN(TAG, "Task ADC saving data >>>>>>>>>>> \t\t %u ms",getTimeDiffInMs(initTime));
        WDT_reset(ADC_ISR);

        vTaskDelayUntil(&initTime, pdMS_TO_TICKS(sampleRateInMS));
    }
}

void IRAM_ATTR adc_mpu9250_fusionTask(void *pvParameter){
    WDT_addTask(FUSION_ISR);
    vTaskDelay(1 / portTICK_PERIOD_MS);

    while (1) {
        TickType_t initTime = xTaskGetTickCount();
        if( takeSensorQueueWhenSamplesAre(MIN_SAMPLES_TO_DO_FUSION)){
            DEBUG_PRINT_MAIN(TAG, "Task Fusion take semaphores >>>>>>>>>>> \t %u ms", getTimeDiffInMs(initTime));

            size_t mpuLength, adcLength;
            QueuePacket_t * mpuBuffer = copyQueueToBuffer(MPUDataQueue,&mpuLength);
            DEBUG_PRINT_MAIN(TAG, "Task Fusion copy buffer mpu >>>>>>>>>>> \t %u ms", getTimeDiffInMs(initTime));

            QueuePacket_t * adcBuffer = copyQueueToBuffer(ADCDataQueue,&adcLength);
            DEBUG_PRINT_MAIN(TAG, "Task Fusion copy buffer adc >>>>>>>>>>> \t %u ms", getTimeDiffInMs(initTime));

            if( mpuBuffer == NULL || adcBuffer == NULL){
                ESP_LOGE(TAG,"Error copying queue to buffer");
                continue;
            }
            giveAllSensorSemaphores();
            DEBUG_PRINT_MAIN(TAG, "Task Fusion give semaphores  >>>>>>>>>>> \t %u ms", getTimeDiffInMs(initTime));

            if ( xSemaphoreTake(xSemaphore_SDMutexQueue, portMAX_DELAY) == pdTRUE) {
                DEBUG_PRINT_MAIN(TAG,"Fusion data Mpu:%d y ADC:%d",mpuLength, adcLength);
                matchSensorsSamples(mpuBuffer, adcBuffer, mpuLength, adcLength);
                xSemaphoreGive(xSemaphore_SDMutexQueue);
            }
            DEBUG_PRINT_MAIN(TAG, "Task Fusion  >>>>>>>>>>> \t %u ms", getTimeDiffInMs(initTime));
            free(mpuBuffer);
            free(adcBuffer);

        }

        WDT_reset(FUSION_ISR);
        vTaskDelayUntil(&initTime, pdMS_TO_TICKS(sampleRateInMS));
    }
}

//
//void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters) {
//
//    ESP_LOGI(TAG,"MPU calibration task init");
//    WDT_addTask(MPU_CALIB_ISR);
//
//    MPU9250_t item;
//    MPU9250_t accumulator;
//
//    while (1) {
//        vTaskDelay(1);
//        // Comprobar si la cola está llena
//        //DEBUG_PRINT(TAG,"MPU Queue space %d",uxQueueMessagesWaiting(MPUDataQueue));
//        if (uxQueueSpacesAvailable(MPUDataQueue) == 0) {
//            if( xSemaphoreTake(xSemaphore_MPUMutexQueue, 1) == pdTRUE) {
//                QueuePacket_t aReceivedPacket;
//                while (xQueueReceive(MPUDataQueue, &aReceivedPacket, 0) == pdTRUE) {
//                    item = getMPUDataFromPacket(aReceivedPacket);
//                    accumulator.Ax += item.Ax;
//                    accumulator.Ay += item.Ay;
//                    accumulator.Az += item.Az;
//                }
//                accumulator.Ax /= QUEUE_LENGTH;
//                accumulator.Ay /= QUEUE_LENGTH;
//                accumulator.Az /= QUEUE_LENGTH;
//
//                DEBUG_PRINT_MAIN(TAG, "MPU Calibration Task %02f %02f %02f", accumulator.Ax,accumulator.Ay,accumulator.Az);
//
//                if( MPU9250_SetCalibrationForAccel(&accumulator) != ESP_OK){
//                    ESP_LOGE(TAG,"Calibration Fail");
//                }else{
//                    ESP_LOGI(TAG,"Calibration Done");
//                }
//                calibrationDone = true;
//                xSemaphoreGive(xSemaphore_MPUMutexQueue);
//            }
//        }
//        WDT_reset(MPU_CALIB_ISR);
//    }
//}

void IRAM_ATTR sd_savingTask(void *pvParameters) {

    ESP_LOGI(TAG,"SD task init");
    WDT_addTask(SD_ISR);
    vTaskDelay(1 / portTICK_PERIOD_MS);

    timeInfo_t timeInfo;
    SD_data_t sdData;
    QueuePacket_t aReceivedPacket;
    SD_data_t sdDataArray[QUEUE_SAMPLE_LENGTH];
    char mainPathToSave[MAX_SAMPLE_PATH_LENGTH];

    DIR_getMainSampleDirectory(mainPathToSave);

    while (1) {
        int sampleCounterToSave = 0;
        TickType_t initTime = xTaskGetTickCount();

        if(uxQueueMessagesWaiting(SDDataQueue) > 0 ){
            if(xSemaphoreTake(xSemaphore_SDMutexQueue, 1) == pdTRUE) {
                DEBUG_PRINT_MAIN(TAG, "Task SD save init  >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));

                while( xQueueReceive(SDDataQueue, &aReceivedPacket, 1)){
                    sdData = getSDDataFromPacket(&aReceivedPacket);
                    //DEBUG_PRINT_MAIN(TAG, "SD Task Received sdData");
                    sample_change_case_t sampleTimeState = analyzeSampleTime(sdData.hour, sdData.min, sdData.seconds);
                    DEBUG_PRINT_MAIN(TAG, "SD Task Received sdData with state %s", printSampleTimeState(sampleTimeState));
                    switch(sampleTimeState){
                        case NEW_MINUTE:
                            if(sampleCounterToSave != 0) {
                                SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                                memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_data_t));
                                sampleCounterToSave = 0;
                            }
                            SD_setSampleFilePath(sdData.hour, sdData.min);
                            break;
                        case NEW_DAY:
                            // Si existe data guardada, implica que correspondía a antes de las 00:00:00
                            // entonces se guarda en el archivo de la fecha anterior.
                            if(sampleCounterToSave != 0) {
                                SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                                memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_data_t));
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
                DEBUG_PRINT_MAIN(TAG, "Task SD saving all data >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));
                SD_writeDataArrayOnSampleFile(sdDataArray, sampleCounterToSave, mainPathToSave);
                DEBUG_PRINT_MAIN(TAG, "Task SD saved all data >>>>>>>>>>> \t\t %u ms", getTimeDiffInMs(initTime));
                memset(sdDataArray, 0, QUEUE_SAMPLE_LENGTH * sizeof(SD_data_t));
                xSemaphoreGive(xSemaphore_SDMutexQueue);
                DEBUG_PRINT_MAIN(TAG, "Task SD finish >>>>>>>>>>> \t\t\t %u ms", getTimeDiffInMs(initTime));

            }
        }
        WDT_reset(SD_ISR);

        vTaskDelayUntil(&initTime, pdMS_TO_TICKS(sampleRateInMS));
    }
}

void IRAM_ATTR mqtt_receiveCommandTask(void *pvParameters){
    ESP_LOGI(TAG,"MQTT receive command task init");
    WDT_addTask(MQTT_RECEIVE_ISR);
    char directoryPathToRetrieve[MAX_SAMPLE_PATH_LENGTH];

    while (1) {
        vTaskDelay(1);
        if (MQTT_HasCommandToProcess()){
            command_t command;
            char rawCommand[MAX_TOPIC_LENGTH];
            MQTT_GetCommand(rawCommand);
            if( COMMAND_Parse(rawCommand, &command) != ESP_OK){
                ESP_LOGE(TAG, "MQTT error parsing command %s", rawCommand);
                continue;
            }
            DEBUG_PRINT_MAIN(TAG, "MQTT Task Received command %s", COMMAND_GetHeaderType(command));
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

                        SD_sensors_data_t *sdData = NULL;
                        if( SD_getDataFromRetrieveSampleFile(directoryPathToRetrieve, &sdData, &totalDataRetrieved) != ESP_OK){
                            MQTT_publish(TOPIC_TO_PUBLISH_DATA, "error reading", strlen("error reading"));
                            ESP_LOGE(TAG,"Error getting data from sample file");
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
                    DEBUG_PRINT_MAIN(TAG,"MQTT Task Finished retrieving data");
                    break;
                }

            }

        }
        WDT_reset(MQTT_RECEIVE_ISR);
    }
}

void IRAM_ATTR mqtt_publishDataTask(void *pvParameters){
    ESP_LOGI(TAG,"WIFI publish task init");
    WDT_addTask(MQTT_PUBLISH_ISR);
    SD_data_t item;
    char dataToSend[MAX_CHARS_PER_SAMPLE];
    char timeHeader[MAX_CHARS_PER_SAMPLE];
    char maxDataToSend[MAX_CHARS_PER_SAMPLE*100];

    int lastMinute = -1, lastHour = -1;
    while (1) {
        vTaskDelay(1);
        if(uxQueueMessagesWaiting(MQTTDataQueue) > 0 ){
            if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, 1) == pdTRUE) {
                MQTT_publish(TOPIC_TO_PUBLISH_DATA, "Start Sending", 13);
                while (uxQueueMessagesWaiting(MQTTDataQueue) != 0){
                    vTaskDelay(1);
                    if( xQueueReceive(MQTTDataQueue, &item, 0) != pdTRUE){
                        DEBUG_PRINT_MAIN(TAG, "Error receiving data from queue");
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
    while (1) {
        vTaskSuspend(TIME_ISR);
        TIME_synchronizeTimeAndDate();
        TIME_printTimeNow();
    }
}


//------------------------------ UTILS -----------------------------------------

esp_err_t ESP32_initQueue() {
    SDDataQueue = xQueueCreate(QUEUE_SAMPLE_LENGTH, QUEUE_SAMPLE_ITEM_SIZE);  if(SDDataQueue == NULL) return ESP_FAIL;
    MQTTDataQueue = xQueueCreate(QUEUE_MQTT_LENGTH, QUEUE_MQTT_SAMPLE_ITEM_SIZE);if(MQTTDataQueue == NULL) return ESP_FAIL;
    ADCDataQueue = xQueueCreate(QUEUE_SAMPLE_LENGTH, QUEUE_SAMPLE_ITEM_SIZE); if(ADCDataQueue == NULL) return ESP_FAIL;
    MPUDataQueue = xQueueCreate(QUEUE_SAMPLE_LENGTH, QUEUE_SAMPLE_ITEM_SIZE); if(MPUDataQueue == NULL) return ESP_FAIL;
    return ESP_OK;
}

esp_err_t ESP32_initSemaphores() {
    xSemaphore_newDataOnMPU = xSemaphoreCreateBinary(); if( xSemaphore_newDataOnMPU == NULL )   return ESP_FAIL;
    xSemaphore_MPUMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_MPUMutexQueue == NULL )  return ESP_FAIL;
    xSemaphore_MQTTMutexQueue = xSemaphoreCreateMutex();if( xSemaphore_MQTTMutexQueue == NULL ) return ESP_FAIL;
    xSemaphore_ADCMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_ADCMutexQueue == NULL )  return ESP_FAIL;
    xSemaphore_SDMutexQueue = xSemaphoreCreateMutex(); if( xSemaphore_SDMutexQueue == NULL )  return ESP_FAIL;
    return ESP_OK;
}

void giveAllSensorSemaphores() {
    xSemaphoreGive(xSemaphore_MPUMutexQueue);
    xSemaphoreGive(xSemaphore_ADCMutexQueue);
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

TickType_t getTimeDiffInMs(TickType_t initTime) {
    TickType_t diff = xTaskGetTickCount() - initTime;
    return pdTICKS_TO_MS(diff);
}


void matchSensorsSamples(QueuePacket_t *mpuBuffer, QueuePacket_t *adcBuffer, size_t mpuLength, size_t adcLength) {
    int mpuIndex = 0;
    int adcIndex = 0;
    while (mpuIndex < mpuLength && adcIndex < adcLength) {
        vTaskDelay(1);
        QueuePacket_t  mpuPacket = mpuBuffer[mpuIndex];
        QueuePacket_t  adcPacket = adcBuffer[adcIndex];

        timeInfo_t timeinfo;
        QueuePacket_t resultPacket;
        DEBUG_PRINT_MAIN(TAG, "MPU tick: %d, ADC tick: %d", mpuPacket.tick, adcPacket.tick);
        int tickDiff = mpuPacket.tick - adcPacket.tick;
        if ( abs(tickDiff) < MIN_TICK_DIFF_TO_MATCH) {
            MPU9250_t mpuData = getMPUDataFromPacket(&mpuPacket);
            ADC_t adcData = getADCDataFromPacket(&adcPacket);
            TIME_getInfoTime(&timeinfo);
            if (buildDataPacketForSD(mpuData, adcData, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,&resultPacket)) {
                if( xQueueSend(SDDataQueue, &resultPacket, portMAX_DELAY) == pdPASS ){
                    DEBUG_PRINT_MAIN(TAG, "\t SD Task Sent item");
                } else {
                    ESP_LOGE(TAG, "Error sending packet to SD queue");
                }
            } else {
                ESP_LOGE(TAG, "Error building packet ... losing samples");
            }
            mpuIndex++;
            adcIndex++;
        } else if (tickDiff > 0) {
            DEBUG_PRINT_MAIN(TAG, "\t Fusion next adc sample...discarding sample");
            getADCDataFromPacket(&adcPacket);
            // if tickDiff is greater than zero, it means that the mpu sample is older than the adc sample
            // so we need to find the adc sample that is closest to the mpu sample
            adcIndex++;
        } else {
            DEBUG_PRINT_MAIN(TAG, "\t Fusion next mpu sample...discarding sample");
            getMPUDataFromPacket(&mpuPacket);
            // else if tickDiff is less than zero, it means that the adc sample is older than the mpu sample
            // so we need to find the mpu sample that is closest to the adc sample
            mpuIndex++;
        }
    }
}

bool takeSensorQueueWhenSamplesAre(int numberOfSamples) {
    UBaseType_t lenOfMPUQueue = uxQueueMessagesWaiting(MPUDataQueue);
    UBaseType_t lenOfADCQueue = uxQueueMessagesWaiting(ADCDataQueue);

    if (lenOfMPUQueue >= numberOfSamples && lenOfADCQueue >= numberOfSamples) {
        if (xSemaphoreTake(xSemaphore_MPUMutexQueue, portMAX_DELAY) == pdTRUE) {
            if ( xSemaphoreTake(xSemaphore_ADCMutexQueue, portMAX_DELAY) == pdTRUE) {
                return true;
            }
        }
    }
    return false;
}

QueuePacket_t * copyQueueToBuffer(QueueHandle_t queue, size_t * bufferLength) {
    size_t queueLength = uxQueueMessagesWaiting(queue);
    if (queueLength == 0){
        DEBUG_PRINT_MAIN(TAG, "Queue is empty");
        *bufferLength = 0;
        return NULL;
    }
    QueuePacket_t * buffer = (QueuePacket_t *) malloc(queueLength * sizeof(QueuePacket_t));
    if (buffer != NULL) {
        QueuePacket_t packet;

        for (size_t i = 0; i < queueLength; i++) {
            if (xQueueReceive(queue, &packet, 0) == pdPASS) {
                memcpy(&buffer[i], &packet, sizeof(QueuePacket_t));
            }
        }
        *bufferLength = queueLength ;
    }
    DEBUG_PRINT_MAIN(TAG, "\t Buffer length copied: %d", *bufferLength);
    return buffer;
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

void pushSDDataToMQTTQueueRoutine(size_t totalDataRetrieved, const SD_sensors_data_t *sdData, int minute, int hour) {
    int sampleSent = 0;
    while(sampleSent != totalDataRetrieved){
        if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, 5) == pdTRUE){
            while(sampleSent < totalDataRetrieved){
                vTaskDelay(1);
                SD_data_t aSDdataToSend;
                memcpy(&aSDdataToSend.sensorsData, &sdData[sampleSent], sizeof(SD_sensors_data_t));
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