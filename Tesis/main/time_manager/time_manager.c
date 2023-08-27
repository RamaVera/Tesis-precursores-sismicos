//
// Created by Ramiro on 5/13/2023.
//

#include "time_manager.h"

static const char *TAG = "TIME ";

TimerHandle_t timerHandle;

esp_err_t TIMER_create(TimerCallbackFunction_t interruptToCallEveryTimelapse) {
    timerHandle = xTimerCreate("timer", pdMS_TO_TICKS(TIMER_PERIOD_MS), pdTRUE, NULL, interruptToCallEveryTimelapse);
    if (timerHandle == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t TIMER_start(void) {
    if (xTimerStart(timerHandle, 0) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t TIME_synchronizeTimeAndDate() {
    ESP_LOGI(TAG, "Getting time from NTP server...");

    // Seteo timezone de Argentina GMT-3
    setenv("TZ", "<-03>3", 1);
    tzset();

    // Configuro el server NTP para sincronizar la hora
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Esperar a que se sincronice la hora
    uint16_t retries;
    for ( retries = 0 ; (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retries < MAX_RETRIES_FOR_SYNC_TIME) ; retries++ ){
        ESP_LOGI(TAG, "Synchronizing time...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    sntp_stop();

    if (retries >= MAX_RETRIES_FOR_SYNC_TIME) {
        ESP_LOGE(TAG, "Max retries reached");
        return ESP_FAIL;
    }
    return ESP_OK;
}

timeInfo_t TIME_getInfoTime(timeInfo_t *timeInfo) {
    time_t now = 0;
    time(&now);
    localtime_r(&now, timeInfo);
    return (*timeInfo);
}

void TIME_printTimeNow(void) {
    timeInfo_t timeinfo;
    TIME_getInfoTime(&timeinfo);
    TIME_printTimeAndDate(&timeinfo);
}

void TIME_printTimeAndDate(timeInfo_t *timeInfo) {
    ESP_LOGI(TAG, "Actual Time is: %s", asctime(timeInfo));
}

esp_err_t TIME_parseParams(char * yearAsString, char * monthAsString, char * dayAsString, timeInfo_t *timeInfo) {
    char * endptr;

    int year = strtol(yearAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting MQTT port");
        return ESP_FAIL;
    }
    int month = strtol(monthAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting MQTT port");
        return ESP_FAIL;
    }
    int day = strtol(dayAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting MQTT port");
        return ESP_FAIL;
    }

    memset(timeInfo, 0, sizeof(*timeInfo));
    timeInfo->tm_year = year - 1900;
    timeInfo->tm_mon = month - 1;
    timeInfo->tm_mday = day;
    return  ESP_OK;
}

void TIME_updateParams(timeInfo_t timeInfo, char * yearAsString, char * monthAsString, char * dayAsString) {
    sprintf(yearAsString, "%d", timeInfo.tm_year + 1900);
    sprintf(yearAsString, "%d", timeInfo.tm_mon + 1);
    sprintf(yearAsString, "%d", timeInfo.tm_mday);
}