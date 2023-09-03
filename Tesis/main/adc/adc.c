
#include "adc.h"

const int ADC_READING_PIN_X = ADC_PIN_X;
const int ADC_READING_PIN_Y = ADC_PIN_Y;
const int ADC_READING_PIN_Z = ADC_PIN_Z;

esp_err_t ADC_Init() {
    // Configurar el modo de operación del ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0);
    adc_set_clk_div(8);
    return ESP_OK;
}

ADC_t ADC_GetRaw() {
    ADC_t adc;
    adc.adcX = (uint16_t)adc1_get_raw(ADC1_CHANNEL_5);
    adc.adcY = (uint16_t)adc1_get_raw(ADC1_CHANNEL_6);
    adc.adcZ = (uint16_t)adc1_get_raw(ADC1_CHANNEL_7);
    return adc;
}
