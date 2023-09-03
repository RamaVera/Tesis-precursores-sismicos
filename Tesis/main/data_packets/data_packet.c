
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

bool buildDataPacketForMPU(MPU9250_t dataToPack, struct QueuePacket *aPacketToGenerate) {
    struct MPU9250_t *mpu9250data = NULL;
    struct QueuePacket aPacket;
    mpu9250data = (struct MPU9250_t *) (malloc(sizeof(struct MPU9250_t)));
    if( mpu9250data == NULL) {
        ESP_LOGE(TAG,"NOT ENOUGH MEMORY");
        return false;
    }
    DEBUG_PRINT_DATA_PACKET(TAG,"MPU Pido memoria para %p",mpu9250data);

    mpu9250data->AxH = dataToPack.AxH;
    mpu9250data->AyH = dataToPack.AyH;
    mpu9250data->AzH = dataToPack.AzH;
    mpu9250data->AxL = dataToPack.AxL;
    mpu9250data->AyL = dataToPack.AyL;
    mpu9250data->AzL = dataToPack.AzL;

    aPacket.dataElement = mpu9250data;
    aPacket.tick = xTaskGetTickCount();
    *aPacketToGenerate = aPacket;

    return true;
}

MPU9250_t getMPUDataFromPacket(QueuePacket_t *aPacket) {
    MPU9250_t mpuRawData;
    MPU9250_t * mpuData = (MPU9250_t *) aPacket->dataElement;
    mpuRawData.AxH = mpuData->AxH;
    mpuRawData.AyH = mpuData->AyH;
    mpuRawData.AzH = mpuData->AzH;
    mpuRawData.AxL = mpuData->AxL;
    mpuRawData.AyL = mpuData->AyL;
    mpuRawData.AzL = mpuData->AzL;
    DEBUG_PRINT_DATA_PACKET(TAG,"MPU libero memoria para %p",aPacket->dataElement);
    free(aPacket->dataElement);

    return mpuRawData;
}

bool buildDataPacketForSD(MPU9250_t mpuRawData, ADC_t adcRawData, int hour, int min, int sec, QueuePacket_t *aPacketToGenerate) {
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

SD_data_t getSDDataFromPacket(QueuePacket_t *aPacket) {
    SD_data_t SDRawData;
    SD_data_t * SDData = (SD_data_t *) aPacket->dataElement;
    SDRawData.sensorsData.mpuData = SDData->sensorsData.mpuData;
    SDRawData.sensorsData.adcData = SDData->sensorsData.adcData;
    SDRawData.hour = SDData->hour;
    SDRawData.min = SDData->min;
    SDRawData.seconds = SDData->seconds;
    DEBUG_PRINT_DATA_PACKET(TAG,"SD libero memoria para %p",aPacket->dataElement);
    free(aPacket->dataElement);

    return SDRawData;
}