/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Vera
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#ifndef SD_CARD_H_
#define SD_CARD_H_

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "../pinout.h"

#define MAX_LENGTH 32
#define MIN_LENGTH 8

typedef struct config_params_t{
    char wifi_ssid[MAX_LENGTH];
    char wifi_password[MAX_LENGTH];
    char mqtt_ip_broker[MAX_LENGTH];
    char mqtt_user[MAX_LENGTH];
    char mqtt_password[MAX_LENGTH];
    char mqtt_port[MAX_LENGTH];
    char init_year[MIN_LENGTH];
    char init_month[MIN_LENGTH];
    char init_day[MIN_LENGTH];
}config_params_t;

/*****************************************************************************
* Prototipos
*****************************************************************************/

esp_err_t SD_init(void);
esp_err_t SD_writeDataOnSampleFile(char dataAsString[], bool withNewLine, char *pathToSave);
esp_err_t SD_getDefaultConfigurationParams(config_params_t *configParams);
esp_err_t SD_writeHeaderToSampleFile(char *pathToSave);
esp_err_t SD_saveLastConfigParams(config_params_t * params);
esp_err_t SD_readLastConfigParams(config_params_t * params);


/*****************************************************************************
* Definiciones
*****************************************************************************/

#define MOUNT_POINT "/sdcard"


// DMA channel to be used by the SPI peripheral
#define SD_SPI_DMA_CHAN    1

#endif /* SD_CARD_H_ */
