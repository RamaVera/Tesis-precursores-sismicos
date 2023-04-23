#include "watchdog.h"

TaskHandle_t IDLE_CORE_ZERO_ISR = NULL;
TaskHandle_t IDLE_CORE_ONE_ISR = NULL;

bool isWDTEnable = true;

void WDT_enableWDTForCores(void){
    esp_task_wdt_init(defaultTimeout, true);
    IDLE_CORE_ZERO_ISR = xTaskGetIdleTaskHandleForCPU(0);
    IDLE_CORE_ONE_ISR = xTaskGetIdleTaskHandleForCPU(1);
    ESP_ERROR_CHECK(esp_task_wdt_add(IDLE_CORE_ZERO_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_add(IDLE_CORE_ONE_ISR));
    isWDTEnable = true;
}

void WDT_disableOnAllCores(void){
    IDLE_CORE_ZERO_ISR = xTaskGetIdleTaskHandleForCPU(0);
    IDLE_CORE_ONE_ISR = xTaskGetIdleTaskHandleForCPU(1);
    ESP_ERROR_CHECK(esp_task_wdt_delete(IDLE_CORE_ZERO_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_delete(IDLE_CORE_ONE_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_deinit());
    isWDTEnable = false;
}

void WDT_add(){
    if( isWDTEnable ){
        TaskHandle_t handle = xTaskGetCurrentTaskHandle();
        ESP_ERROR_CHECK(esp_task_wdt_add(handle));
    }
}

void WDT_remove(){
    if( isWDTEnable ) {
        TaskHandle_t handle = xTaskGetCurrentTaskHandle();
        ESP_ERROR_CHECK(esp_task_wdt_delete(handle));
    }
}

void WDT_reset(){
    if( isWDTEnable ){
        // Si el watchdog esta habilitado y se llamo a reset y devuelve
        // not found es que no se agrego correctamente en el task
        // se vuelve a agregar
      if ( esp_task_wdt_reset() == ESP_ERR_NOT_FOUND ) {
          WDT_add();
      }
    }
}


