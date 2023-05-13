/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
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

#define PERM_READ S_IRUSR
#define PERM_WRITE S_IWUSR
#define PERM_EXECUTION S_IXUSR
#define PERM_GROUP_READ S_IRGRP
#define PERM_GROUP_WRITE S_IWGRP
#define PERM_GROUP_EXECUTION S_IXGRP
#define PERM_OTHER_READ S_IROTH
#define PERM_OTHER_WRITE S_IWOTH
#define PERM_OTHER_EXECUTION S_IXOTH
#define PERM_ADMIN (PERM_READ | PERM_WRITE | PERM_EXECUTION | PERM_GROUP_READ | PERM_GROUP_WRITE | PERM_GROUP_EXECUTION |PERM_OTHER_READ | PERM_OTHER_WRITE | PERM_OTHER_EXECUTION)

typedef struct Config_params_t{
    char wifi_ssid[MAX_LENGTH];
    char wifi_password[MAX_LENGTH];
    char mqtt_ip_broker[MAX_LENGTH];
    char mqtt_user[MAX_LENGTH];
    char mqtt_password[MAX_LENGTH];
    char mqtt_port[MAX_LENGTH];
}Config_params_t;

/*****************************************************************************
* Prototipos
*****************************************************************************/

esp_err_t SD_init(void);
esp_err_t SD_writeData(char dataAsString[], bool withNewLine);
esp_err_t SD_getInitialParams(Config_params_t *configParams);
esp_err_t SD_createInitialFiles();


/*****************************************************************************
* Definiciones
*****************************************************************************/
#define MOUNT_POINT "/sdcard"
#define SAMPLE_PATH MOUNT_POINT"/samples"

// DMA channel to be used by the SPI peripheral
#define SD_SPI_DMA_CHAN    1

#endif /* SD_CARD_H_ */
