/** \file	data_packet.h
 *  \brief
 *  Autor: Ramiro Vera
 *  Versión: 1
 *
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <malloc.h>
#include <freertos/portmacro.h>
#include "esp_log.h"
#include "../adc/adc.h"
#include "../mpu_9250/mpu9250.h"


typedef struct QueuePacket{
    void * dataElement;
    TickType_t tick;
}QueuePacket_t;

typedef struct SD_data {
    MPU9250_t mpuData;
    ADC_t adcData;
    int hour;
    int seconds;
    int min;
}SD_data_t;

/*****************************************************************************
* Prototipos
*****************************************************************************/
bool buildDataPacketForADC(int adcRawData, QueuePacket_t *aPacketToGenerate);
struct ADC_t getADCDataFromPacket(QueuePacket_t aPacket);

bool buildDataPacketForMPU(float rawAx, float rawAy, float rawAz, QueuePacket_t *aPacketToGenerate);
MPU9250_t getMPUDataFromPacket(QueuePacket_t aPacket);

bool buildDataPacketForSD(MPU9250_t mpuRawData, ADC_t adcRawData, int hour, int min, int sec, QueuePacket_t *aPacketToGenerate);
SD_data_t getSDDataFromPacket(QueuePacket_t aPacket);

/*****************************************************************************
* Definiciones
*****************************************************************************/


#endif /* PACKET_H_ */