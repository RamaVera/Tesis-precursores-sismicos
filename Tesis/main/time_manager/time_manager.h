//
// Created by Ramiro on 5/13/2023.
//

#ifndef RTC_H
#define RTC_H

#define TIMER_PERIOD_MS (1 * 1000)


#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>

#include "lwip/apps/sntp.h"
#include <sys/time.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>

void RTC_printTimeNow();
esp_err_t RTC_configureTimer(TimerCallbackFunction_t interruptToCallEveryTimelapse);
esp_err_t RTC_startTimer(void);
void RTC_sincronizeTimeAndDate(void);
#endif //RTC_H
