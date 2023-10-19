#ifndef WDT_H
#define WDT_H

#include "esp_task_wdt.h"

#define SECONDS 1
#define WATCHDOG_TIMEOUT_IN_SECONDS 100
static const int defaultTimeout = WATCHDOG_TIMEOUT_IN_SECONDS * SECONDS; // 5 segs

void WDT_enableOnAllCores(void);
void WDT_disableOnAllCores(void);
void WDT_addTask(TaskHandle_t handle);
void WDT_removeTask(TaskHandle_t handle);
void WDT_reset(TaskHandle_t handle);



#endif //WDT_H
