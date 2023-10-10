//
// Created by Ramiro on 5/13/2023.
//

#ifndef RTC_H
#define RTC_H

#define TIMER_TO_REFRESH_HOUR_PERIOD_MS (60 * 60 * 1000) // 1hrs timer
#define TIMER_DEFAULT_SAMPLE_PERIOD_MS 200

#define MAX_RETRIES_FOR_SYNC_TIME 15

typedef struct timeInfo_t{
    int milliseconds; // Milisegundos (0-999)
    int tm_sec;      // Segundos (0-59)
    int tm_min;      // Minutos (0-59)
    int tm_hour;     // Horas (0-23)
    int tm_mday;     // Día del mes (1-31)
    int tm_mon;      // Mes (0-11, donde 0 es enero)
    int tm_year;     // Año - 1900
    int tm_wday;     // Día de la semana (0-6, donde 0 es domingo)
    int tm_yday;     // Día del año (0-365)
    int tm_isdst;    // Horario de verano (1 si está en horario de verano, 0 si no lo está, -1 si es desconocido)
} timeInfo_t;

#include "freertos/FreeRTOS.h"
#include <freertos/timers.h>

#include <esp_sntp.h>
#include "lwip/apps/sntp.h"
#include <sys/time.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>


esp_err_t TIMER_create(int32_t periodInMS, TimerCallbackFunction_t interruptToCallEveryTimelapse);
esp_err_t TIMER_start(void);
esp_err_t TIME_synchronizeTimeAndDate();
void TIME_printTimeAndDate(timeInfo_t *timeInfo);
esp_err_t TIME_parseParams(char * yearAsString, char * monthAsString, char * dayAsString, timeInfo_t *timeInfo);
void TIME_printTimeNow(void);
timeInfo_t TIME_getInfoTime(timeInfo_t *timeInfo);
void TIME_updateParams(timeInfo_t timeInfo, char * yearAsString, char * monthAsString, char * dayAsString);

#endif //RTC_H
