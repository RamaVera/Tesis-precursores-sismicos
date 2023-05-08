
#include "adc.h"


esp_err_t ADC_Init() {
    // Configurar el modo de operación del ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);
    adc_set_clk_div(8);
    return ESP_OK;
}

int ADC_GetRaw() {
    return adc1_get_raw(ADC1_CHANNEL_5);
}
