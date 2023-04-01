/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#ifndef SD_CARD_H_
#define SD_CARD_H_

/*****************************************************************************
* Prototipos
*****************************************************************************/

esp_err_t SD_init(void);
esp_err_t SD_writeData(char dataAsString[], bool withNewLine);

/*****************************************************************************
* Definiciones
*****************************************************************************/
#define MOUNT_POINT "/sdcard"

#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK  18
#define SD_PIN_NUM_CS   5

// DMA channel to be used by the SPI peripheral
#define SD_SPI_DMA_CHAN    1

#endif /* SD_CARD_H_ */
