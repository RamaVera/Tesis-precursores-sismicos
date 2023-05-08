//
// Created by Ramiro on 4/16/2023.
//

#ifndef ADC_H
#define ADC_H

#include "driver/adc.h"
#include "esp_err.h"

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

typedef struct ADC_t{
    int data;
}ADC_t ;

esp_err_t ADC_Init();
int ADC_GetRaw();


#endif //ADC_H
