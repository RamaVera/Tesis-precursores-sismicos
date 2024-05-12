#include "watchdog.h"

esp_err_t WDT_init (uint32_t timeoutInSeconds ){
	esp_err_t err = esp_task_wdt_init( timeoutInSeconds, true);
	return err;
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


