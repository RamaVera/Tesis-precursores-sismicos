#include "watchdog.h"

TaskHandle_t IDLE_CORE_ZERO_ISR = NULL;
TaskHandle_t IDLE_CORE_ONE_ISR = NULL;

void WDT_enableOnAllCores(void){
    esp_task_wdt_init(defaultTimeout, true);
    IDLE_CORE_ZERO_ISR = xTaskGetIdleTaskHandleForCPU(0);
    IDLE_CORE_ONE_ISR = xTaskGetIdleTaskHandleForCPU(1);
    ESP_ERROR_CHECK(esp_task_wdt_add(IDLE_CORE_ZERO_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_add(IDLE_CORE_ONE_ISR));
}

void WDT_disableOnAllCores(void){
    IDLE_CORE_ZERO_ISR = xTaskGetIdleTaskHandleForCPU(0);
    IDLE_CORE_ONE_ISR = xTaskGetIdleTaskHandleForCPU(1);
    ESP_ERROR_CHECK(esp_task_wdt_delete(IDLE_CORE_ZERO_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_delete(IDLE_CORE_ONE_ISR));
    ESP_ERROR_CHECK(esp_task_wdt_deinit());
}

void WDT_addTask(TaskHandle_t handle) {
    ESP_ERROR_CHECK(esp_task_wdt_add(handle));
}

void WDT_removeTask(TaskHandle_t handle) {
    ESP_ERROR_CHECK(esp_task_wdt_delete(handle));
}

void WDT_reset(TaskHandle_t handle) {
    ESP_ERROR_CHECK(esp_task_wdt_reset());
}


