/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */

#ifndef SD_CARD_H_
#define SD_CARD_H_

#include "../main.h"

/*****************************************************************************
* Prototipos
*****************************************************************************/

void inicializacion_tarjeta_SD(void);
void extraccion_tarjeta_SD(void);
void borrar_datos_SD(void);
void leer_config_SD (void);


/*****************************************************************************
* Definiciones
*****************************************************************************/
#define MOUNT_POINT "/sdcard"

// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
#define SPI_DMA_CHAN    1
#endif //SPI_DMA_CHAN



#endif /* SD_CARD_H_ */
