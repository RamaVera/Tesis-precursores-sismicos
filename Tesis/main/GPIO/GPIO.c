 /** \file	GPIOS.c
  *  \brief	Contiene las funciones de manejo e inicializacion de los GPIOs
  *  Autor: Ramiro Alonso
  *  Versión: 1
  *	Contiene las funciones de manejo e inicializacion de los GPIOs
  */

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"

#include "GPIO.h"

/************************************************************************
* Variables
************************************************************************/
//extern volatile xQueueHandle gpio_evt_queue = NULL;
volatile uint32_t ciclos_seg;
volatile uint32_t ciclos_int;
volatile uint32_t ciclos_ant;

/**
 * @brief Handler de interrupcion de los GPIOS
 */

/*
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    ciclos_int = xthal_get_ccount();
    // ciclos_seg = ciclos_int - ciclos_ant;
    //
    // char aux_msg1[20];
    // char aux_msg2[20];

//    sprintf(aux_msg1, "%u", ciclos_seg);
//    sprintf(aux_msg2, "%u", ciclos_int);

    // printf("Ciclos / seg: %d  Ciclo actual: %d \n\r", ciclos_seg, ciclos_int);
    //
    // ciclos_ant = ciclos_int;

    uint32_t gpio_num = (uint32_t) arg;
//    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
*/



/**
 * @brief Inicialización de los GPIO
 */

int inicializacion_gpios(void){



    gpio_config_t io_conf;

  // CONFIGURO PINES DE SALIDA
      //disable interrupt
      io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
      //set as output mode
      io_conf.mode = GPIO_MODE_OUTPUT;
      //bit mask of the pins that you want to set,e.g.GPIO18/19
      io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;  // Configura ambas salidas (ver #define)
      //disable pull-down mode
      io_conf.pull_down_en = 0;
      //disable pull-up mode
      io_conf.pull_up_en = 0;
      //configure GPIO with the given settings
      gpio_config(&io_conf);


  // CONFIGURO PINES DE ENTRADA
    //interrupt of rising edge
//    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;

    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_pull_mode(0, GPIO_FLOATING);   // CMD, needed in 4- and 1- line modes



  //    gpio_pullup_dis(4);

  //   //change gpio intrrupt type for one pin
  //   gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);
  //
  //   //install gpio isr service
  //   gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  //   //hook isr handler for specific gpio pin
  //   gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
  //   //hook isr handler for specific gpio pin
  // //  gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);




    return 0;
}
