#ifndef WDT_H
#define WDT_H

#include "esp_task_wdt.h"

static const int defaultTimeout = 5; // 5 segs

void WDT_enableOnAllCores(void);
void WDT_disableOnAllCores(void);
void WDT_add();
void WDT_remove();
void WDT_reset();



#endif //WDT_H
