
#include "data_packet.h"

static const char *TAG = "DATA_PACKET"; // Para los mensajes de LOG

//#define DEBUG_DATA_PACKET
#ifdef DEBUG_DATA_PACKET
#define DEBUG_PRINT_DATA_PACKET(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT_DATA_PACKET(tag, fmt, ...) do {} while (0)
#endif

bool buildDataPacketForADC(int adcRawData, struct QueuePacket *aPacketToGenerate) {
    struct ADC_t *adcData = NULL;
    struct QueuePacket aPacket;
    adcData = (ADC_t *) (malloc(sizeof(ADC_t)));
    if( adcData == NULL){
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"ADC Pido memoria para %p",adcData);
    adcData->data = adcRawData;
    aPacket.dataElement = adcData;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

ADC_t getADCDataFromPacket(struct QueuePacket aPacket) {
    ADC_t adcRawData;
    ADC_t * adcData = (ADC_t *) aPacket.dataElement;
    adcRawData.data = adcData->data;
    DEBUG_PRINT_DATA_PACKET(TAG,"ADC libero memoria para %p",aPacket.dataElement);
    free(aPacket.dataElement);
    return adcRawData;
}

bool buildDataPacketForMPU(float rawAx, float rawAy, float rawAz, struct QueuePacket *aPacketToGenerate) {
    struct MPU9250_t *mpu9250data = NULL;
    struct QueuePacket aPacket;
    mpu9250data = (struct MPU9250_t *) (malloc(sizeof(struct MPU9250_t)));
    if( mpu9250data == NULL) {
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"MPU Pido memoria para %p",mpu9250data);

    mpu9250data->Ax = rawAx;
    mpu9250data->Ay = rawAy;
    mpu9250data->Az = rawAz;
    aPacket.dataElement = mpu9250data;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

struct MPU9250_t getMPUDataFromPacket(struct QueuePacket aPacket) {
    MPU9250_t mpuRawData;
    MPU9250_t * mpuData = (MPU9250_t *) aPacket.dataElement;
    mpuRawData.Ax = mpuData->Ax;
    mpuRawData.Ay = mpuData->Ay;
    mpuRawData.Az = mpuData->Az;
    DEBUG_PRINT_DATA_PACKET(TAG,"MPU libero memoria para %p",aPacket.dataElement);
    free(aPacket.dataElement);
    return mpuRawData;
}

bool buildDataPacketForSD(struct MPU9250_t mpuRawData, struct ADC_t adcRawData, int hour, int min, int sec, struct QueuePacket *aPacketToGenerate) {
    SD_data_t *SDdata = NULL;
    QueuePacket_t aPacket;
    SDdata = (SD_data_t *) (malloc(sizeof(SD_data_t)) );
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

SD_data_t getSDDataFromPacket(struct QueuePacket aPacket) {
    SD_data_t SDRawData;
    SD_data_t * SDData = (SD_data_t *) aPacket.dataElement;
    SDRawData.sensorsData.mpuData = SDData->sensorsData.mpuData;
    SDRawData.sensorsData.adcData = SDData->sensorsData.adcData;
    SDRawData.hour = SDData->hour;
    SDRawData.min = SDData->min;
    SDRawData.seconds = SDData->seconds;
    DEBUG_PRINT_DATA_PACKET(TAG,"SD libero memoria para %p",aPacket.dataElement);
    free(aPacket.dataElement);
    return SDRawData;
}