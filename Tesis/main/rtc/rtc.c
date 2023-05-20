//
// Created by Ramiro on 5/13/2023.
//

#include "rtc.h"


static const char *TAG = "RTC "; // Para los mensajes de LOG

TimerHandle_t timerHandle;
int yearSeed,monthSeed,daySeed;

esp_err_t str2int(const char *string,int * number);

esp_err_t RTC_setSeed(char * yearString, char * monthString, char * dayString){
    int year,month,day;
    if( str2int(yearString,&year) != ESP_OK) {
       return ESP_FAIL;
    }
    if( str2int(monthString,&month) != ESP_OK) {
        return ESP_FAIL;
    }
    if( str2int(dayString,&day) != ESP_OK ){
        return ESP_FAIL;
    }
    yearSeed = year;
    monthSeed = month;
    daySeed = day;
    return ESP_OK;
}

esp_err_t str2int(const char *string,int * number) {
    char *endptr;
    *number = strtol(string, &endptr, 10);
    if (endptr == string) {
        ESP_LOGE(TAG,"No se ha encontrado ningún número válido.\n");
        return ESP_FAIL;
    } else if (*endptr != '\0') {
        ESP_LOGE(TAG,"Se encontró un número válido parcial, pero también hay caracteres adicionales: %s\n", endptr);
        return ESP_FAIL;

    } else {
        ESP_LOGI(TAG,"Número convertido: %d\n", *number);
        return ESP_OK;
    }
}

void RTC_printTimeNow(){
    struct tm timeinfo;

    time_t now = 0;
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG,"%d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo.tm_year + yearSeed, timeinfo.tm_mon + monthSeed, timeinfo.tm_mday + daySeed,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}


esp_err_t RTC_configureTimer(TimerCallbackFunction_t interruptToCallEveryTimelapse) {
    timerHandle = xTimerCreate("timer", pdMS_TO_TICKS(TIMER_PERIOD_MS), pdTRUE, NULL, interruptToCallEveryTimelapse);
    if (timerHandle == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t RTC_startTimer(void) {
    if (xTimerStart(timerHandle, 0) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

