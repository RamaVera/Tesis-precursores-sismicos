/** \file	GPIOS.h
 *  \brief	Contiene las funciones de manejo e inicializacion de los GPIOs
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 */

#ifndef GPIO_H_
#define GPIO_H_

/*****************************************************************************
* Prototipos
*****************************************************************************/

 //static void IRAM_ATTR gpio_isr_handler(void* arg);
 int inicializacion_gpios(void);


/*****************************************************************************
* Definiciones
*****************************************************************************/

// Entradas
#define BOTON_1     27  // Entrada en GPIO 27  → BOTON 1
#define BOTON_2     26  // Entrada en GPIO 26  → BOTON 2
#define CARD_SENSE  5

//salidas
#define LED_1    33 // LED_1
#define LED_2    25 // LED_2

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_1) | (1ULL<<LED_2))

#define GPIO_INPUT_PIN_SEL   ((1ULL<<BOTON_1) | (1ULL<<BOTON_2) | (1ULL<<CARD_SENSE))

#define ESP_INTR_FLAG_DEFAULT 0

#endif /* GPIO_H_ */
