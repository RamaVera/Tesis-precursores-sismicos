/*
 * wifi.h
 *
 *  Created on: May 9, 2020
 *      Author: jaatadia@gmail.com
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


#define WIFI_SSID "Casita"        //SSID WIFI (Nombre de red)
#define WIFI_PASS "MushuMisty94"        //Contraseña WIFI
#define EXAMPLE_ESP_MAXIMUM_RETRY  10


esp_err_t WIFI_init(void);
esp_err_t WIFI_connect(void);



#endif /* MAIN_WIFI_H_ */
