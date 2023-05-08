/*
 * main.c
 *
 *  Created on: Jan 01, 2023
 *      Author: rverag@fi.uba.ar
 */
#include "main.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(tag, fmt, ...) do {} while (0)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) do {} while (0)
#endif
/************************************************************************
* Variables Globales
************************************************************************/
#define QUEUE_LENGTH 50
#define QUEUE_ITEM_SIZE sizeof(QueuePacket_t)

const char SD_LINE_PATTERN[30] = "%02f\t%02f\t%02f\t%d";
const char SD_LINE_PATTERN_WITH_NEW_LINE[30] ="%02f\t%02f\t%02f\t%d\n";

TaskHandle_t MPU_ISR = NULL;
TaskHandle_t MPU_CALIB_ISR = NULL;
TaskHandle_t SD_ISR = NULL;
TaskHandle_t ADC_ISR = NULL;
TaskHandle_t WIFI_PUBLISH_ISR = NULL;

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

static const char *TAG = "MAIN "; // Para los mensajes del micro

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters);

void IRAM_ATTR adc_readingTask(void *pvParameters);
void IRAM_ATTR mpu9250_readingTask(void *pvParameters);
void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters);
void IRAM_ATTR adc_mpu9250_fusionTask(void *pvParameter);
void IRAM_ATTR sd_savingTask(void *pvParameters);
void IRAM_ATTR wifi_publishDataTask(void *pvParameters);


void CorrelateDataAndSendToSDQueue(UBaseType_t lenOfMPUQueue, UBaseType_t lenOfADCQueue);

void takeAllSensorSemaphores();

void giveAllSensorSemaphores();

void app_main(void) {

    defineLogLevels();
    status_t nextStatus = INITIATING;
    while(nextStatus != DONE){
        printStatus(nextStatus);

        switch (nextStatus) {
            case INITIATING: {
                if (Button_init() != ESP_OK) return;
                if (SD_init() != ESP_OK) return;
                if (ADC_Init() != ESP_OK) return;
                if (MPU9250_init() != ESP_OK) return;
                if (WIFI_init() != ESP_OK) return;
                if (MPU9250_attachInterruptWith(mpu9250_enableReadingTaskByInterrupt, false) != ESP_OK) return;
                if (ESP32_initSemaphores() != ESP_OK) return;
                if (ESP32_initQueue() != ESP_OK) return;
                if (WIFI_connect() == ESP_OK ) {
                    if (MQTT_init() != ESP_OK) return;
                }
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
                    nextStatus = SETTING_UP;
                }
                break;
            }
            case SETTING_UP: {
                static bool creatingTask = true;
                if (creatingTask) {
                    creatingTask = false;
                    xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &MPU_ISR, 0);
                    xTaskCreatePinnedToCore(mpu9250_calibrationTask, "mpu9250_calibrationTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_CALIB_ISR, 1);
                    MPU9250_enableInterrupt(true);
                }

                bool firstTimeForCalibration = true;

                if( calibrationDone && firstTimeForCalibration) {
                    firstTimeForCalibration = false;
                    MPU9250_enableInterrupt(false);
                    vTaskDelay(10/portTICK_PERIOD_MS);
                    WDT_removeTask(MPU_ISR);
                    WDT_removeTask(MPU_CALIB_ISR);
                    vTaskDelete(MPU_ISR);
                    vTaskDelete(MPU_CALIB_ISR);

                    DEBUG_PRINT(TAG,"---------------Finish Calibration--------------");
                    nextStatus = INIT_SAMPLING;
                }
                break;
            }
            case INIT_SAMPLING: {
                xTaskCreatePinnedToCore(adc_readingTask, "adc_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &ADC_ISR, 1);
                xTaskCreatePinnedToCore(mpu9250_readingTask, "mpu9250_readingTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 3, &MPU_ISR, 1);
                xTaskCreatePinnedToCore(sd_savingTask, "sd_savingTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, &SD_ISR,0);
                xTaskCreatePinnedToCore(adc_mpu9250_fusionTask, "adc_mpu9250_fusionTask", 1024 * 16, NULL, tskIDLE_PRIORITY + 2, NULL,0);
                //xTaskCreatePinnedToCore(wifi_publishDataTask, "wifi_publishDataTask", 1024 * 16, NULL,tskIDLE_PRIORITY + 2, &WIFI_PUBLISH_ISR, 1);
                MPU9250_enableInterrupt(true);
                nextStatus = DONE;
                break;
            }

            case DONE:
            case ERROR:
            default:
                 break;
        }
    }
}

void printStatus(status_t nextStatus) {
    static status_t lastStatus = ERROR;

    if( lastStatus != nextStatus) {
        DEBUG_PRINT(TAG, "--------------%s-----------------\n",statusAsString[nextStatus]);
        lastStatus = nextStatus;
    }
}

void defineLogLevels() {
    esp_log_level_set("MAIN", ESP_LOG_VERBOSE );
    esp_log_level_set("MPU9250", ESP_LOG_INFO );
    esp_log_level_set("SD_CARD", ESP_LOG_INFO );
    esp_log_level_set("WIFI", ESP_LOG_ERROR );
    esp_log_level_set("MQTT", ESP_LOG_INFO );
    esp_log_level_set("MENSAJES_MQTT", ESP_LOG_INFO );
    esp_log_level_set("HTTP_FILE_SERVER", ESP_LOG_ERROR );
}

//----------------------------INTERRUPTIONS -----------------------------------

void IRAM_ATTR mpu9250_enableReadingTaskByInterrupt(void* pvParameters){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore_newDataOnMPU, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    DEBUG_PRINT_INTERRUPT(">>>>> \n");
}

//------------------------------ TASKS -----------------------------------------

void IRAM_ATTR mpu9250_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "Mpu task init");
    WDT_addTask(MPU_ISR);
    while (1) {
        vTaskDelay(1);
        // xSemaphore_newDataOnMPU toma el semaforo principal.
        // Solo se libera si la interrupcion de nuevo dato disponible lo libera.
        // Para que la interrupcion se de, tiene que llamarse a mpu9250_ready
        if ( xSemaphoreTake(xSemaphore_newDataOnMPU, portMAX_DELAY) != pdTRUE ){
            continue;
        }

        //DEBUG_PRINT(TAG, "MPU Task Global > take semaphore");
        MPU9250_t data;
        if( MPU9250_ReadAcce(&data)  != ESP_OK){
            ESP_LOGE(TAG, "MPU Task Fail getting data");
            xSemaphoreGive(xSemaphore_newDataOnMPU);
            continue;
        }
        DEBUG_PRINT(TAG, "MPU Task Received item %02f %02f %02f", data.Ax,data.Ay,data.Az);

        // xSemaphore_queue toma el semaforo para que se acceda a la cola
        if(xSemaphoreTake(xSemaphore_MPUMutexQueue, portMAX_DELAY) == pdTRUE){
            QueuePacket_t aPacket;
            if( !buildDataPacketForMPU(data.Ax,data.Ay,data.Az,&aPacket) ){
                ESP_LOGE(TAG,"Error building packet");
                xSemaphoreGive(xSemaphore_newDataOnMPU);
                continue;
            }
            xQueueSend(MPUDataQueue, &aPacket, portMAX_DELAY);
            xSemaphoreGive(xSemaphore_MPUMutexQueue);
        }
        // Avisa que el dato fue leido por lo que baja el pin de interrupcion esperando que se haga un nuevo
        // flanco ascendente
        mpu9250_ready();
        WDT_reset(MPU_ISR);
    }
}

void IRAM_ATTR mpu9250_calibrationTask(void *pvParameters) {

    ESP_LOGI(TAG,"MPU calibration task init");
    WDT_addTask(MPU_CALIB_ISR);

    MPU9250_t item;
    MPU9250_t accumulator;

    while (1) {
        vTaskDelay(1);
        // Comprobar si la cola está llena
        //DEBUG_PRINT(TAG,"MPU Queue space %d",uxQueueMessagesWaiting(MPUDataQueue));
        if (uxQueueSpacesAvailable(MPUDataQueue) == 0) {
            if( xSemaphoreTake(xSemaphore_MPUMutexQueue, portMAX_DELAY) == pdTRUE) {
                QueuePacket_t aReceivedPacket;
                while (xQueueReceive(MPUDataQueue, &aReceivedPacket, 0) == pdTRUE) {
                    item = getMPUDataFromPacket(aReceivedPacket);
                    accumulator.Ax += item.Ax;
                    accumulator.Ay += item.Ay;
                    accumulator.Az += item.Az;
                }
                accumulator.Ax /= QUEUE_LENGTH;
                accumulator.Ay /= QUEUE_LENGTH;
                accumulator.Az /= QUEUE_LENGTH;

                DEBUG_PRINT(TAG, "MPU Calibration Task %02f %02f %02f", accumulator.Ax,accumulator.Ay,accumulator.Az);

                if( MPU9250_SetCalibrationForAccel(&accumulator) != ESP_OK){
                    ESP_LOGI(TAG,"Calibration Fail");
                }else{
                    ESP_LOGI(TAG,"Calibration Done");
                }
                calibrationDone = true;
                xSemaphoreGive(xSemaphore_MPUMutexQueue);
            }
        }
        WDT_reset(MPU_CALIB_ISR);
    }
}

void IRAM_ATTR sd_savingTask(void *pvParameters) {

    ESP_LOGI(TAG,"SD task init");
    WDT_addTask(SD_ISR);

    SD_data_t sdData;
    QueuePacket_t aReceivedPacket;
    char data[200];
    bool newLine = true;

    sprintf(data, "Ax\t\tAy\t\tAz\t\tADC\t\t");
    SD_writeData(data, newLine);

    while (1) {
        vTaskDelay(1);
        if(uxQueueMessagesWaiting(SDDataQueue) > 0 ){
            if(xSemaphoreTake(xSemaphore_SDMutexQueue, 1) == pdTRUE) {
                while(uxQueueMessagesWaiting(SDDataQueue) != 0){
                    if (xQueueReceive(SDDataQueue, &aReceivedPacket, 0) == pdTRUE){
                        sdData = getSDDataFromPacket(aReceivedPacket);
                        DEBUG_PRINT(TAG, "SD Task Received sdData");
                        char * printPattern;
                        if (uxQueueMessagesWaiting(SDDataQueue)>0){
                            printPattern = (char *) &SD_LINE_PATTERN_WITH_NEW_LINE;
                        }else{
                            printPattern = (char *) &SD_LINE_PATTERN;
                        }
                        sprintf(data, printPattern,
                                sdData.mpuData.Ax,
                                sdData.mpuData.Ay,
                                sdData.mpuData.Az,
                                sdData.adcData.data);
                    }
                }
                SD_writeData(data, newLine);
                xSemaphoreGive(xSemaphore_SDMutexQueue);
            }
        }
        WDT_reset(SD_ISR);
    }
}

void IRAM_ATTR wifi_publishDataTask(void *pvParameters){
    ESP_LOGI(TAG,"Wifi publish task init");
    WDT_addTask(WIFI_PUBLISH_ISR);
    MPU9250_t item;

    while (1) {
        vTaskDelay(1);
        if(xSemaphoreTake(xSemaphore_MQTTMutexQueue, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(MQTTDataQueue, &item, 0) == pdTRUE){
                DEBUG_PRINT(TAG, "MQTT Task Received item");
                cJSON *jsonAccelData;
                jsonAccelData = cJSON_CreateObject();
                cJSON_AddNumberToObject(jsonAccelData, "accel_x", item.Ax);
                cJSON_AddNumberToObject(jsonAccelData, "accel_y", item.Ay);
                cJSON_AddNumberToObject(jsonAccelData, "accel_z", item.Az);

                char * rendered;
                rendered = cJSON_Print(jsonAccelData);

                MQTT_publish("datos",rendered,strlen(rendered));
                cJSON_Delete(jsonAccelData);
            }
            xSemaphoreGive(xSemaphore_MQTTMutexQueue);
        }
        WDT_reset(WIFI_PUBLISH_ISR);
    }
}

void IRAM_ATTR adc_readingTask(void *pvParameters) {

    ESP_LOGI(TAG, "adc task init");
    WDT_addTask(ADC_ISR);

    while (1) {
        vTaskDelay(1);
        int data = ADC_GetRaw();
        DEBUG_PRINT(TAG, "ADC Task Received item %d", data);

        if(xSemaphoreTake(xSemaphore_ADCMutexQueue, portMAX_DELAY) == pdTRUE){
            QueuePacket_t aPacket;
            if( !buildDataPacketForADC(data,&aPacket) ){
                ESP_LOGE(TAG,"Error building packet");
                continue;
            }
            xQueueSend(ADCDataQueue, &aPacket, portMAX_DELAY);
            xSemaphoreGive(xSemaphore_ADCMutexQueue);
        }
        WDT_reset(ADC_ISR);
    }
}

void IRAM_ATTR adc_mpu9250_fusionTask(void *pvParameter){

    while (1) {
        vTaskDelay(1);
        takeAllSensorSemaphores();
        UBaseType_t lenOfMPUQueue = uxQueueMessagesWaiting(MPUDataQueue);
        UBaseType_t lenOfADCQueue = uxQueueMessagesWaiting(ADCDataQueue);
        DEBUG_PRINT(TAG,"Fusion data Mpu:%d y ADC:%d",lenOfMPUQueue,lenOfADCQueue);
        CorrelateDataAndSendToSDQueue(lenOfMPUQueue, lenOfADCQueue);
        giveAllSensorSemaphores();
    }
}

//------------------------------ UTILS -----------------------------------------

void CorrelateDataAndSendToSDQueue(UBaseType_t lenOfMPUQueue, UBaseType_t lenOfADCQueue) {
    QueueHandle_t longestQueue,shorterQueue;
    if (lenOfMPUQueue > lenOfADCQueue){
        longestQueue = MPUDataQueue;
        shorterQueue = ADCDataQueue;
    } else {
        longestQueue = ADCDataQueue;
        shorterQueue = MPUDataQueue;
    }

    // Buscamos el par de elementos con la diferencia mínima de tick
    QueuePacket_t aPacketFromShortQueue,aPacketFromLongQueue,resultPacket;
    MPU9250_t mpuData;
    ADC_t adcData;
    TickType_t minDiff;

    while ( uxQueueMessagesWaiting(shorterQueue) > 0 ) {
        minDiff = portMAX_DELAY ;
        xQueueReceive(shorterQueue, &aPacketFromShortQueue, 1);
        if (lenOfMPUQueue > lenOfADCQueue){
            adcData = getADCDataFromPacket(aPacketFromShortQueue);
        } else {
            mpuData = getMPUDataFromPacket(aPacketFromShortQueue);
        }

        while ( uxQueueMessagesWaiting(longestQueue) > 0) {
            xQueuePeek(longestQueue, &aPacketFromLongQueue, 1);
            TickType_t tickDiff = tickAbsDiff( aPacketFromLongQueue.tick,aPacketFromShortQueue.tick);

            if (tickDiff <= minDiff ) {
                // Si esto se repite mas de una vez, la anterior muestra se descarta pues esto indica
                // que la segunda muestra esta mas correlacionada que la primera
                xQueueReceive(longestQueue, &aPacketFromLongQueue, 1);
                if (lenOfMPUQueue > lenOfADCQueue){
                    mpuData = getMPUDataFromPacket(aPacketFromLongQueue);
                }else{
                    adcData = getADCDataFromPacket(aPacketFromLongQueue);
                }
                minDiff = tickDiff;
            } else {
                break;
            }

        }

        buildDataPacketForSD(mpuData,adcData,&resultPacket);
        xQueueSend(SDDataQueue, &resultPacket, portMAX_DELAY);
    }
}

esp_err_t ESP32_initQueue() {
    SDDataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);  if( SDDataQueue     == NULL) return ESP_FAIL;
    MQTTDataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);if( MQTTDataQueue   == NULL) return ESP_FAIL;
    ADCDataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE); if( ADCDataQueue    == NULL) return ESP_FAIL;
    MPUDataQueue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE); if( MPUDataQueue    == NULL) return ESP_FAIL;
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
    xSemaphoreGive(xSemaphore_SDMutexQueue);
}

void takeAllSensorSemaphores() {
    bool bothQueueAreEmpty = pdFALSE;
    bool mpuAlreadyTaken = false;
    bool adcAlreadyTaken = false;
    bool sdAlreadyTaken = false;
    while (!bothQueueAreEmpty){
        vTaskDelay(1);
        UBaseType_t lenOfMPUQueue = uxQueueMessagesWaiting(MPUDataQueue);
        UBaseType_t lenOfADCQueue = uxQueueMessagesWaiting(ADCDataQueue);

        if( lenOfMPUQueue > 0 && lenOfADCQueue > 0 ){
            if( !mpuAlreadyTaken ){
                if ( xSemaphoreTake(xSemaphore_MPUMutexQueue, portMAX_DELAY) != pdTRUE){
                    continue;
                } else{
                    mpuAlreadyTaken = true;
                }
            }
            if( !adcAlreadyTaken ){
                if ( xSemaphoreTake(xSemaphore_ADCMutexQueue, portMAX_DELAY) != pdTRUE){
                    continue;
                }else{
                    adcAlreadyTaken = true;
                }
            }
            if( !sdAlreadyTaken ){
                if ( xSemaphoreTake(xSemaphore_SDMutexQueue, portMAX_DELAY) != pdTRUE){
                    continue;
                } else{
                    sdAlreadyTaken = true;
                }
            }
            if (mpuAlreadyTaken && adcAlreadyTaken && sdAlreadyTaken ){
                bothQueueAreEmpty = pdTRUE;
            }
        }
    }
}

TickType_t tickAbsDiff(TickType_t tick1, TickType_t tick2) {
    if (tick1 >= tick2) {
        return tick1 - tick2;
    } else {
        return tick2 - tick1;
    }
}
