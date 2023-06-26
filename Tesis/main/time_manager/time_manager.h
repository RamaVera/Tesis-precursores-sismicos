//
// Created by Ramiro on 5/13/2023.
//

#ifndef RTC_H
#define RTC_H

#define TIMER_PERIOD_MS (60 * 60 * 1000) // 1hrs timer

#define MAX_RETRIES_FOR_SYNC_TIME 15

typedef struct tm timeInfo_t;

#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>

#include <esp_sntp.h>
#include "lwip/apps/sntp.h"
#include <sys/time.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>


esp_err_t RTC_configureTimer(TimerCallbackFunction_t interruptToCallEveryTimelapse);
esp_err_t RTC_startTimer(void);
esp_err_t TIME_synchronizeTimeAndDate();
void TIME_printTimeAndDate(struct tm *timeInfo);
esp_err_t TIME_parseParams(char * yearAsString, char * monthAsString, char * dayAsString, timeInfo_t *timeInfo);
void TIME_printTimeNow(void);
struct tm TIME_getInfoTime(struct tm *timeInfo);
void TIME_updateParams(timeInfo_t timeInfo, char * yearAsString, char * monthAsString, char * dayAsString);

#endif //RTC_H
