
#include "adc.h"


esp_err_t ADC_Init() {
    // Configurar el modo de operación del ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_0);
    return ESP_OK;
}