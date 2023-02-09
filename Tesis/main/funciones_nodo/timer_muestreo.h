/** \file	timer_muestreo.h
 *  \brief	Contiene las funciones de inicializacion y el handler del timer
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 */

#ifndef TIMER_MUESTREO_H_
#define TIMER_MUESTREO_H_

/*****************************************************************************
* Definiciones
*****************************************************************************/
#define TIMER_DIVIDER         2 //  Hardware timer clock divider (2 es el valor mínimo), cuenta a 40MHz
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

/*****************************************************************************
* Prototipos
*****************************************************************************/

void IRAM_ATTR ISR_Handler_timer_muestreo(void *ptr);
void inicializacion_timer_muestreo(int timer_idx, bool auto_reload, uint64_t valor_max_conteo);


#endif /* TIMER_MUESTREO_H_ */
