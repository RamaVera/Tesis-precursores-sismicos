//
// Created by Ramiro on 5/13/2023.
//

#include "time_manager.h"

static const char *TAG = "TIME "; // Para los mensajes de LOG

TimerHandle_t timerHandle;

void RTC_printTimeNow(){
    struct tm timeinfo;

    time_t now = 0;
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Hora actual: %s", strftime_buf);
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


void RTC_sincronizeTimeAndDate(void)
{
    ESP_LOGI(TAG, "Obteniendo la hora del servidor NTP...");
    setenv("TZ", "GMT-3", 1);
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // Espera hasta que se obtenga la hora actual
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while ((timeinfo.tm_year + 1900) < 2020 && ++retry < retry_count) {
        ESP_LOGI(TAG, "Esperando la hora actual...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // Mostrar la hora obtenida
    if (retry < retry_count) {
        setenv("TZ", "GMT-3", 1);
        tzset();
        time(&now);
        localtime_r(&now, &timeinfo);
        // Configurar el RTC interno con la hora obtenida
        struct timeval tv;
        tv.tv_sec = now;
        tv.tv_usec = 0;

        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "Hora actual: %s", asctime(&timeinfo));
    } else {
        ESP_LOGE(TAG, "Error al obtener la hora del servidor NTP");
    }
    sntp_stop();
}

