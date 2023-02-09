/** \file	timer_muestreo.c
 *  \brief	Contiene las funciones de inicializacion y el handler del timer
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de los GPIOs
 *
 *  IMPORTANTE: EL HANDLER Y TODAS LAS FUNCIONES LLAMADAS DENTRO DE LA INTERRUPCIÓN SE TIENEN QUE UBICAR EN LA IRAM:
 *   ** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/general-notes.html **
 *
 */

#include <stdio.h>
#include "../main.h"
#include "../tictoc/daemon.h"
#include "esp_log.h"
//#include "driver/i2c.h"
//#include "sdkconfig.h"
//#include "driver/timer.h"
#include "timer_muestreo.h"
#include "GPIO.h"
#include "esp_attr.h"

/************************************************************************
* Variables externas
************************************************************************/

extern SemaphoreHandle_t xSemaphore_tomamuestra;
extern SemaphoreHandle_t xSemaphore_guardatabla;

extern muestreo_t Datos_muestreo;
extern mensaje_t mensaje_consola;
extern uint8_t LED;

/************************************************************************
* Variables
************************************************************************/

volatile uint64_t valor_interrupcion_timer;

extern TicTocData * ticTocData;

/**
 * @brief Handler de la interrupcion del Timer
 * El timer cuenta a 40MHz
 */
void IRAM_ATTR ISR_Handler_timer_muestreo(void *ptr)
{
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        switch (Datos_muestreo.estado_muestreo) {

        case ESTADO_ESPERANDO_MENSAJE_DE_INICIO:    // Si estoy esperando el mensaje de inicio
                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer);

                break;

        case ESTADO_CONFIGURAR_ALARMA_INICIO_A:  // Configuro timer para que interrumpa un segundo antes del inicio
                if(ticTocReady(ticTocData)) {
                        int64_t ttTime_irq;
                        ttTime_irq = ticTocTime(ticTocData);
                        timer_set_alarm_value(TIMER_GROUP_0, 0, (Datos_muestreo.epoch_inicio - ttTime_irq)*40-40000000);         // Setea la alarma para que la proxima interrupcion sea en el inicio. (40 cuentas por us)

        #ifdef MOSTRAR_MENSAJES
                        sprintf(mensaje_consola.mensaje,"Alarma A configurada, Epoch_inicio: %lld    tt_Time1:%lld  Tiempo_timer: %lld \n", Datos_muestreo.epoch_inicio, ttTime_irq,(Datos_muestreo.epoch_inicio - ttTime_irq)*40-40000000 );
                        mensaje_consola.mensaje_nuevo=true;
        #endif
                        Datos_muestreo.estado_muestreo = ESTADO_CONFIGURAR_ALARMA_INICIO_B;         // En la proxima interrupcion empiezo a muestrear
                }
                else{         // Dejo la interrupción en un periodo genérico
                        timer_set_alarm_value(TIMER_GROUP_0, 0, (40000000/MUESTRAS_POR_SEGUNDO));
                }
                break;

        case ESTADO_CONFIGURAR_ALARMA_INICIO_B:         // Configuro timer para que interrumpa en el inicio del muestreo

                if(ticTocReady(ticTocData)) {
                        int64_t ttTime_irq;
                        ttTime_irq = ticTocTime(ticTocData);
                        timer_set_alarm_value(TIMER_GROUP_0, 0, (Datos_muestreo.epoch_inicio - ttTime_irq)*40);                 // Setea la alarma para que la proxima interrupcion sea en el inicio. (40 cuentas por us)

                #ifdef MOSTRAR_MENSAJES
                        sprintf(mensaje_consola.mensaje,"Alarma B configurada, Falta 1 segundo, Epoch_inicio: %lld    tt_Time1:%lld  Tiempo_timer: %lld \n", Datos_muestreo.epoch_inicio, ttTime_irq,(Datos_muestreo.epoch_inicio - ttTime_irq)*40 );
                        mensaje_consola.mensaje_nuevo=true;
                #endif
                        Datos_muestreo.estado_muestreo = ESTADO_MUESTREANDO;                 // En la proxima interrupcion empiezo a muestrear
                }
                else{                 // Dejo la interrupción en un periodo genérico
                        timer_set_alarm_value(TIMER_GROUP_0, 0, (40000000/MUESTRAS_POR_SEGUNDO));
                }
                break;

        case ESTADO_MUESTREANDO:

                // if (LED == 0) {
                //         gpio_set_level(LED_1, 1);
                //         LED=1;
                // }
                // else {
                //         LED=0;
                //         gpio_set_level(LED_1, 0);
                // }
                Datos_muestreo.cantidad_de_interrupciones_de_muestreo++;
                xSemaphoreGiveFromISR( xSemaphore_tomamuestra, &xHigherPriorityTaskWoken );

                if (Datos_muestreo.flag_tomar_muestra == true) {         // Si es true es porque no se leyó la muestra anterior.
                        Datos_muestreo.flag_muestra_perdida = true;
                }
                Datos_muestreo.flag_tomar_muestra = true;

                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer); // Valor de reseteo por defecto del timer. Según frecuencia de muestreo definida

// RESINCRONIZAMOS AL FINALIZAR EL SEGUNDO

                if ( ((Datos_muestreo.cantidad_de_interrupciones_de_muestreo-1)-(Datos_muestreo.int_contador_segundos*MUESTRAS_POR_SEGUNDO)) >= (MUESTRAS_POR_SEGUNDO-1)) {           // Al finalizar las muestras del segundo vuelvo a sincronizar.

                        Datos_muestreo.int_contador_segundos++;         // Aumento 1 al contador de segundos

                        if (Datos_muestreo.int_contador_segundos >= Datos_muestreo.duracion_muestreo) {         // Si terminé de muestrear
                                //resetea_muestreo();
                                Datos_muestreo.estado_muestreo = ESTADO_FINALIZANDO_MUESTREO;         // Vuelvo al estado de espera de instrucciones

        #ifdef MOSTRAR_MENSAJES
                                sprintf(mensaje_consola.mensaje,"Fin de muestreo nro: %d | Interrupciones: %d | Muestras leidas: %d \n", Datos_muestreo.nro_muestreo, Datos_muestreo.cantidad_de_interrupciones_de_muestreo, Datos_muestreo.cantidad_de_muestras_leidas );
                                mensaje_consola.mensaje_nuevo=true;
        #endif
                        }

                        else if(ticTocReady(ticTocData)) { // Sincronizo para el próximo segundo de muestreo
                                int64_t ttTime_irq;
                                ttTime_irq = ticTocTime(ticTocData);
                                timer_set_alarm_value(TIMER_GROUP_0, 0, (Datos_muestreo.epoch_inicio + Datos_muestreo.int_contador_segundos*1000000L - ttTime_irq)*40);         // Setea la alarma para que la proxima interrupcion sea en el inicio.
        #ifdef MOSTRAR_MENSAJES
                                sprintf(mensaje_consola.mensaje,"Epoch_inicio: %lld   Inicio_prox_seg:%lld  Cantidad de ciclos: %lld \n", Datos_muestreo.epoch_inicio, (Datos_muestreo.epoch_inicio + Datos_muestreo.int_contador_segundos*1000000L - ttTime_irq), (Datos_muestreo.epoch_inicio + Datos_muestreo.int_contador_segundos*1000000L - ttTime_irq)*40);
                                mensaje_consola.mensaje_nuevo=true;
        #endif
                        }
                }
                break;

        case ESTADO_FINALIZANDO_MUESTREO:

                Datos_muestreo.flag_fin_muestreo = true; // Para mandar el mensaje de confirmacion por mqtt

                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer); // Valor de reseteo por defecto del timer. Según frecuencia de muestreo definida

                Datos_muestreo.nro_muestra_total_muestreo = 0;         // Contador de muestras totales en el muestreo.
                //xSemaphoreGiveFromISR( xSemaphore_tomamuestra, &xHigherPriorityTaskWoken ); // Una última muestra para cerrar el archivo.
                Datos_muestreo.estado_muestreo = ESTADO_ESPERANDO_MENSAJE_DE_INICIO;         // Vuelvo al estado de espera de instrucciones

                break;


        case ESTADO_MUESTREANDO_ASYNC:

                Datos_muestreo.cantidad_de_interrupciones_de_muestreo++;
                xSemaphoreGiveFromISR( xSemaphore_tomamuestra, &xHigherPriorityTaskWoken );

                if (Datos_muestreo.flag_tomar_muestra == true) {                 // Si es true es porque no se leyó la muestra anterior.
                        Datos_muestreo.flag_muestra_perdida = true;
                }
                Datos_muestreo.flag_tomar_muestra = true;

                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer);         // Valor de reseteo por defecto del timer. Según frecuencia de muestreo definida

                if ( ((Datos_muestreo.cantidad_de_interrupciones_de_muestreo-1)-(Datos_muestreo.int_contador_segundos*MUESTRAS_POR_SEGUNDO)) >= (MUESTRAS_POR_SEGUNDO-1)) {                   // Al finalizar las muestras del segundo vuelvo a sincronizar.

                        Datos_muestreo.int_contador_segundos++;                 // Aumento 1 al contador de segundos

                        if (Datos_muestreo.int_contador_segundos >= Datos_muestreo.duracion_muestreo) {                 // Si terminé de muestrear
                                //resetea_muestreo();
                                Datos_muestreo.estado_muestreo = ESTADO_FINALIZANDO_MUESTREO;                 // Vuelvo al estado de espera de instrucciones


          #ifdef MOSTRAR_MENSAJES
                                sprintf(mensaje_consola.mensaje,"Fin de muestreo nro: %d | Interrupciones: %d | Muestras leidas: %d \n", Datos_muestreo.nro_muestreo, Datos_muestreo.cantidad_de_interrupciones_de_muestreo, Datos_muestreo.cantidad_de_muestras_leidas );
                                mensaje_consola.mensaje_nuevo=true;
          #endif
                        }

                        else if(ticTocReady(ticTocData)) {         // Sincronizo para el próximo segundo de muestreo
                                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer);   // Valor de reseteo por defecto del timer. Según frecuencia de muestreo definida

                        }
                }
                break;

        default:
                timer_set_alarm_value(TIMER_GROUP_0, 0, valor_interrupcion_timer);
                break;




        }



// Si está sincronizado prendemos el LED verde y ponemos en hora el reloj local
        if(ticTocReady(ticTocData)) {
                gpio_set_level(LED_2, 1);
                // int64_t ttTime_irq;
                // ttTime_irq = ticTocTime(ticTocData);
                // struct timeval current_time1;
                // current_time1.tv_sec = ttTime_irq/1000000;
                // current_time1.tv_usec = 0;
                // settimeofday(&current_time1, NULL);
        }
        else{
                gpio_set_level(LED_2, 0);
        }




        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
        timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

        if(xHigherPriorityTaskWoken == pdTRUE) {
                portYIELD_FROM_ISR ();         // Solicito cambio de contexto
        }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Inicializacion del timer 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
void inicializacion_timer_muestreo(int timer_idx, bool auto_reload, uint64_t valor_max_conteo)
{
        /* Select and initialize basic parameters of the timer */
        timer_config_t config;
        config.divider = TIMER_DIVIDER;
        config.counter_dir = TIMER_COUNT_UP;
        config.counter_en = TIMER_PAUSE;
        config.alarm_en = TIMER_ALARM_EN;
        config.intr_type = TIMER_INTR_LEVEL;
        config.auto_reload = auto_reload;
 #ifdef TIMER_GROUP_SUPPORTS_XTAL_CLOCK
        config.clk_src = TIMER_SRC_CLK_APB;
 #endif
        timer_init(TIMER_GROUP_0, timer_idx, &config);

        /* El timer inicia el conteo en el siguiente valor.
           Si se configura en "auto reload", se reinicia en el siguiente valor.*/
        timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0);

        /* Configure the alarm value and the interrupt on alarm. */
        timer_set_alarm_value(TIMER_GROUP_0, timer_idx, valor_max_conteo);

        valor_interrupcion_timer = valor_max_conteo;
        timer_enable_intr(TIMER_GROUP_0, timer_idx);
        timer_isr_register(TIMER_GROUP_0, timer_idx, ISR_Handler_timer_muestreo,
                           (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

        timer_start(TIMER_GROUP_0, timer_idx);
}
