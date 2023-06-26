//
// Created by Ramiro on 4/16/2023.
//

#ifndef ADC_H
#define ADC_H

#include "driver/adc.h"
#include "../pinout.h"

esp_err_t ADC_Init();
int ADC_GetRaw();


#endif //ADC_H
