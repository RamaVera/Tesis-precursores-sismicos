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

typedef struct SD_data_t {
    MPU9250_t mpuData;
    ADC_t adcData;
}SD_data_t;

/*****************************************************************************
* Prototipos
*****************************************************************************/
bool buildDataPacketForADC(int adcRawData, struct QueuePacket *aPacketToGenerate);
struct ADC_t getADCDataFromPacket(struct QueuePacket aPacket);

bool buildDataPacketForMPU(float rawAx, float rawAy, float rawAz, struct QueuePacket *aPacketToGenerate);
struct MPU9250_t getMPUDataFromPacket(struct QueuePacket aPacket);

bool buildDataPacketForSD(struct MPU9250_t mpuRawData, struct ADC_t adcRawData, struct QueuePacket *aPacketToGenerate);
struct SD_data_t getSDDataFromPacket(struct QueuePacket aPacket);

/*****************************************************************************
* Definiciones
*****************************************************************************/


#endif /* PACKET_H_ */