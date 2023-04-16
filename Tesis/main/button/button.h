/** \file	GPIOS.h
 *  \brief	Contiene las funciones de manejo e inicializacion de los GPIOs
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "esp_log.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "esp_err.h"

#include "../pinout.h"

/*****************************************************************************
* Prototipos
*****************************************************************************/
esp_err_t Button_init();
bool Button_isPressed();
esp_err_t Button_attachInterruptWith(gpio_isr_t functionToDoWhenRiseAnInterrupt );

/*****************************************************************************
* Definiciones
*****************************************************************************/


#endif /* GPIO_H_ */
