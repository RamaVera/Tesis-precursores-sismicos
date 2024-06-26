/** \file	data_packet.h
 *  \brief
 *  Autor: Ramiro Vera
 *  Versión: 1
 *
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <malloc.h>
#include "esp_log.h"
#include "../data_types.h"
#include <esp_event.h>


typedef struct QueuePacket{
    void * dataElement;
    TickType_t tick;
}QueuePacket_t;

/*****************************************************************************
* Prototipos
*****************************************************************************/
bool buildDataPacketForADC(uint16_t adcX, uint16_t adcY, uint16_t adcZ, QueuePacket_t *aPacketToGenerate);
ADC_t getADCDataFromPacket(QueuePacket_t *aPacket);

bool buildDataPacketForMPU(MPU9250_t dataToPack, QueuePacket_t *aPacketToGenerate);
MPU9250_t getMPUDataFromPacket(QueuePacket_t *aPacket);

bool buildDataPacketForSD(MPU9250_t mpuRawData, ADC_t adcRawData, int hour, int min, int sec, QueuePacket_t *aPacketToGenerate);
SD_time_t getSDDataFromPacket(QueuePacket_t *aPacket);

/*****************************************************************************
* Definiciones
*****************************************************************************/


#endif /* PACKET_H_ */