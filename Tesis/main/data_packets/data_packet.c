
#include "data_packet.h"

static const char *TAG = "DATA_PACKET"; // Para los mensajes de LOG

#define DEBUG_DATA_PACKET
#ifdef DEBUG_DATA_PACKET
#define DEBUG_PRINT(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) ets_printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(tag, fmt, ...) do {} while (0)
#define DEBUG_PRINT_INTERRUPT(fmt, ...) do {} while (0)
#endif

bool buildDataPacketForADC(int adcRawData, struct QueuePacket *aPacketToGenerate) {
    struct ADC_t *adcData = NULL;
    struct QueuePacket aPacket;
    adcData = (ADC_t *) (malloc(sizeof(ADC_t)));
    if( adcData == NULL){
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT(TAG,"ADC Pido memoria para %p",adcData);

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
    DEBUG_PRINT(TAG,"ADC libero memoria para %p",aPacket.dataElement);
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
    DEBUG_PRINT(TAG,"MPU Pido memoria para %p",mpu9250data);

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
    DEBUG_PRINT(TAG,"MPU libero memoria para %p",aPacket.dataElement);
    free(aPacket.dataElement);
    return mpuRawData;
}

bool buildDataPacketForSD(MPU9250_t mpuRawData, ADC_t adcRawData, struct QueuePacket *aPacketToGenerate) {
    struct SD_data_t *SDdata = NULL;
    struct QueuePacket aPacket;
    SDdata = (struct SD_data_t *) (malloc(sizeof(struct SD_data_t)) );
    if( SDdata == NULL){
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT(TAG,"SD Pido memoria para %p",SDdata);

    SDdata->adcData = adcRawData;
    SDdata->mpuData = mpuRawData;
    aPacket.dataElement = SDdata;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

struct SD_data_t getSDDataFromPacket(struct QueuePacket aPacket) {
    SD_data_t SDRawData;
    SD_data_t * SDData = (SD_data_t *) aPacket.dataElement;
    SDRawData.mpuData = SDData->mpuData;
    SDRawData.adcData = SDData->adcData;
    DEBUG_PRINT(TAG,"SD libero memoria para %p",aPacket.dataElement);
    free(aPacket.dataElement);
    return SDRawData;
}