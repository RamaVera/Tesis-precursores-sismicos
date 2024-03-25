//
// Created by Ramiro on 4/16/2023.
//

#ifndef ADC_H
#define ADC_H

#include "driver/adc.h"
#include "../data_types.h"
#include "../pinout.h"

esp_err_t ADC_Init();
esp_err_t ADC_GetRaw ( ADC_t *adc );


#endif //ADC_H
