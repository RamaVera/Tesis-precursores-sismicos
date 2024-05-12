#ifndef WDT_H
#define WDT_H

#include "esp_task_wdt.h"

#define WATCHDOG_TIMEOUT_IN_SECONDS 5

esp_err_t WDT_init (uint32_t timeoutInSeconds );
void WDT_addTask(TaskHandle_t handle);
void WDT_removeTask(TaskHandle_t handle);
void WDT_reset(TaskHandle_t handle);



#endif //WDT_H
