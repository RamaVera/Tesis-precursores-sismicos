//
// Created by Ramiro on 5/13/2023.
//

#ifndef RTC_H
#define RTC_H

#define TIMER_PERIOD_MS (10 * 1000)


#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>

#include <sys/time.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>

void RTC_printTimeNow();
esp_err_t RTC_setSeed(char * yearString, char * monthString, char * dayString);
esp_err_t RTC_configureTimer(TimerCallbackFunction_t interruptToCallEveryTimelapse);
esp_err_t RTC_startTimer(void);
#endif //RTC_H
