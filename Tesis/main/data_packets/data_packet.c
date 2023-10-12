
#include "data_packet.h"

static const char *TAG = "DATA_PACKET"; // Para los mensajes de LOG

//#define DEBUG_DATA_PACKET
#ifdef DEBUG_DATA_PACKET
#define DEBUG_PRINT_DATA_PACKET(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_DATA_PACKET(tag, fmt, ...) do {} while (0)
#endif

bool buildDataPacketForADC(uint16_t adcX, uint16_t adcY, uint16_t adcZ, QueuePacket_t *aPacketToGenerate) {
    ADC_t *adcData = NULL;
    QueuePacket_t aPacket;
    adcData = (ADC_t *) (malloc(sizeof(ADC_t)));
    if( adcData == NULL){
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"ADC Pido memoria para %p",adcData);
    adcData->adcX = adcX;
    adcData->adcY = adcY;
    adcData->adcZ = adcZ;
    aPacket.dataElement = adcData;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

ADC_t getADCDataFromPacket(QueuePacket_t *aPacket) {
    ADC_t adcRawData;
    ADC_t * adcData = (ADC_t *) aPacket->dataElement;
    adcRawData.adcX = adcData->adcX;
    adcRawData.adcY = adcData->adcY;
    adcRawData.adcZ = adcData->adcZ;
    DEBUG_PRINT_DATA_PACKET(TAG,"ADC libero memoria para %p",aPacket->dataElement);
    free(aPacket->dataElement);
    return adcRawData;
}

bool buildDataPacketForMPU(MPU9250_t dataToPack, QueuePacket_t *aPacketToGenerate) {
    MPU9250_t *mpu9250data = NULL;
    QueuePacket_t aPacket;
    mpu9250data = (MPU9250_t *) (malloc(sizeof(MPU9250_t)));
    if( mpu9250data == NULL) {
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"MPU Pido memoria para %p",mpu9250data);

    mpu9250data->accelX = dataToPack.accelX;
    mpu9250data->accelY = dataToPack.accelY;
    mpu9250data->accelZ = dataToPack.accelZ;

    aPacket.dataElement = mpu9250data;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

MPU9250_t getMPUDataFromPacket(QueuePacket_t *aPacket) {
    MPU9250_t mpuRawData;
    MPU9250_t * mpuData = (MPU9250_t *) aPacket->dataElement;
    mpuRawData.accelX = mpuData->accelX;
    mpuRawData.accelY = mpuData->accelY;
    mpuRawData.accelZ = mpuData->accelZ;

    DEBUG_PRINT_DATA_PACKET(TAG,"MPU libero memoria para %p",aPacket->dataElement);
    free(aPacket->dataElement);

    return mpuRawData;
}

bool buildDataPacketForSD(MPU9250_t mpuRawData, ADC_t adcRawData, int hour, int min, int sec, QueuePacket_t *aPacketToGenerate) {
    SD_time_t *SDdata = NULL;
    QueuePacket_t aPacket;
    SDdata = (SD_time_t *) (malloc(sizeof(SD_time_t)) );
    if( SDdata == NULL){
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"SD Pido memoria para %p",SDdata);

    SDdata->sensorsData.adcData = adcRawData;
    SDdata->sensorsData.mpuData = mpuRawData;
    SDdata->hour = hour;
    SDdata->min = min;
    SDdata->seconds = sec;
    aPacket.dataElement = SDdata;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

SD_time_t getSDDataFromPacket(QueuePacket_t *aPacket) {
    SD_time_t SDRawData;
    SD_time_t * SDData = (SD_time_t *) aPacket->dataElement;
    SDRawData.sensorsData.mpuData = SDData->sensorsData.mpuData;
    SDRawData.sensorsData.adcData = SDData->sensorsData.adcData;
    SDRawData.hour = SDData->hour;
    SDRawData.min = SDData->min;
    SDRawData.seconds = SDData->seconds;
    DEBUG_PRINT_DATA_PACKET(TAG,"SD libero memoria para %p",aPacket->dataElement);
    free(aPacket->dataElement);

    return SDRawData;
}