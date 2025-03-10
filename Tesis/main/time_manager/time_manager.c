// Created by Ramiro on 5/13/2023.
//

#include "time_manager.h"

static const char *TAG = "TIME ";

esp_err_t TIMER_create (char *name, int32_t periodInMS, TimerCallbackFunction_t interruptToCallEveryTimelapse, TimerHandle_t *handle ) {
	TimerHandle_t timeHandle;
	timeHandle = xTimerCreate(name, pdMS_TO_TICKS(periodInMS), pdTRUE, NULL, interruptToCallEveryTimelapse);
    if (timeHandle == NULL) {
        return ESP_FAIL;
    }
	*handle = timeHandle;
    return ESP_OK;
}

esp_err_t TIMER_start ( TimerHandle_t timerHandle ) {
    if (xTimerStart(timerHandle, 1) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t TIME_synchronizeTimeAndDateFromInternet () {
    ESP_LOGI(TAG, "Getting time from NTP server...");
	
    // Configuro el server NTP para sincronizar la hora
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Esperar a que se sincronice la hora
    uint16_t retries;
    for ( retries = 0 ; (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retries < MAX_RETRIES_FOR_SYNC_TIME) ; retries++ ){
        ESP_LOGI(TAG, "Synchronizing time...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    sntp_stop();

    if (retries >= MAX_RETRIES_FOR_SYNC_TIME) {
        ESP_LOGE(TAG, "Max retries reached");
        return ESP_FAIL;
    }
    return ESP_OK;
}

timeval_t TIME_saveSnapshot (timeval_t *timeInfo ) {
	struct timezone tz_argentina = { .tz_minuteswest = -180, .tz_dsttime = 0 };
	gettimeofday(timeInfo, &tz_argentina);
	return *timeInfo;
}


timeInfo_t TIME_getInfoTime(timeInfo_t *timeInfo) {
    time_t now = 0;
    time(&now);
    struct tm tm_time;
    struct timeval tv;
	gettimeofday(&tv, NULL);
	
	// Adjust the time to GMT-3
	setenv("TZ", "ART3", 1);
	tzset();
    localtime_r(&now, &tm_time);

    // Copia los valores a tu propia estructura timeInfo_t
    timeInfo->tm_sec = tm_time.tm_sec;
    timeInfo->tm_min = tm_time.tm_min;
    timeInfo->tm_hour = tm_time.tm_hour;
    timeInfo->tm_mday = tm_time.tm_mday;
    timeInfo->tm_mon = tm_time.tm_mon;
    timeInfo->tm_year = tm_time.tm_year;
    timeInfo->tm_wday = tm_time.tm_wday;
    timeInfo->tm_yday = tm_time.tm_yday;
    timeInfo->tm_isdst = tm_time.tm_isdst;
    timeInfo->milliseconds = tv.tv_usec / 1000;

    return *timeInfo;
}

void TIME_printTimeNow(void) {
    timeInfo_t timeinfo;
    TIME_getInfoTime(&timeinfo);
    TIME_printTimeAndDate(&timeinfo);
}

void TIME_printTimeAndDate(timeInfo_t *timeInfo) {
    struct tm tm_time;

    // Copia los valores relevantes de timeInfo a tm_time
    tm_time.tm_sec = timeInfo->tm_sec;
    tm_time.tm_min = timeInfo->tm_min;
    tm_time.tm_hour = timeInfo->tm_hour;
    tm_time.tm_mday = timeInfo->tm_mday;
    tm_time.tm_mon = timeInfo->tm_mon;
    tm_time.tm_year = timeInfo->tm_year;
    tm_time.tm_wday = timeInfo->tm_wday;
    tm_time.tm_yday = timeInfo->tm_yday;
    tm_time.tm_isdst = timeInfo->tm_isdst;

    ESP_LOGI(TAG, "Actual Time is: %s", asctime(&tm_time));
}

esp_err_t TIME_parseParams ( timeInfo_t *timeInfo, char *yearAsString, char *monthAsString, char *dayAsString ) {
    char * endptr;

    int year = strtol(yearAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting Offline Year Seed - %s", yearAsString);
        return ESP_FAIL;
    }
    int month = strtol(monthAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting Offline Month seed - %s", monthAsString);
        return ESP_FAIL;
    }
    int day = strtol(dayAsString, &endptr, 10);
    if (*endptr != '\0') {
        ESP_LOGE(TAG,"Error Getting Offline Day seed - %s", dayAsString);
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

void TIME_DiffNow (timeInfo_t *actual, timeval_t *init ) {
	timeval_t end;
	TIME_saveSnapshot( &end );
	TIME_Diff( actual, init, &end );
}

void TIME_Diff( timeInfo_t *actual, timeval_t *init, timeval_t *end ) {
	actual->tm_mday = abs(end->tv_sec - init->tv_sec) / (60 * 60 * 24);
	actual->tm_hour = abs((end->tv_sec - init->tv_sec) % (60 * 60 * 24)) / (60 * 60);
	actual->tm_min = abs((end->tv_sec - init->tv_sec) % (60 * 60)) / 60;
	actual->tm_sec = abs((end->tv_sec - init->tv_sec) % 60);
	actual->milliseconds = abs((end->tv_usec - init->tv_usec) / 1000);
	actual->microseconds = abs((end->tv_usec - init->tv_usec) % 1000);
}
