#include "button.h"

esp_err_t Button_init() {
    gpio_pad_select_gpio(BOTON_1);
    gpio_set_direction(BOTON_1, GPIO_MODE_INPUT);
    gpio_pulldown_en(BOTON_1);
    gpio_pullup_dis(BOTON_1);
    return ESP_OK;
}

esp_err_t Button_attachInterruptWith(gpio_isr_t functionToDoWhenRiseAnInterrupt ){
    if(functionToDoWhenRiseAnInterrupt != NULL){
        gpio_set_intr_type(BOTON_1, GPIO_INTR_POSEDGE);
        gpio_install_isr_service(0);
        if( gpio_isr_handler_add(MPU_PIN_NUM_INT, functionToDoWhenRiseAnInterrupt, NULL ) != ESP_OK){ return ESP_FAIL;} ;
    }
    return ESP_OK;
}


bool Button_isPressed(void){
    return (gpio_get_level(BOTON_1) == 1);
}