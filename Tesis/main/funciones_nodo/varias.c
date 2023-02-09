/** \file	varias.c
 *  \brief	Contiene tareas varias del programa
 *  Autor: Ramiro Alonso
 *  Versión: 1
 */

#include "../main.h"
#include "../tictoc/daemon.h"
#include "../tictoc/microtime.h"

#include "mqtt.h"
#include "tareas.h"

#define MENSAJES_MQTT

extern FILE *f_samples;
extern SemaphoreHandle_t xSemaphore_mutex_archivo;
extern muestreo_t Datos_muestreo;
extern uint8_t LED;


static const char *TAG = "VARIAS "; // Para los mensajes del micro

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FUNCION PARA RESETEAR LAS VARIABLES DE MUESTREO Y DEJAR TODO LISTO PARA EMPEZAR OTRO ////////////////////////////////////////
void resetea_muestreo(void){
        Datos_muestreo.epoch_inicio=0; //Evitamos un inicio no deseado
        Datos_muestreo.estado_muestreo=ESTADO_ESPERANDO_MENSAJE_DE_INICIO; // Pausamos la toma de datos
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Para evitar que se tomen muestras mientra borro las variables
        Datos_muestreo.selec_tabla_escritura = 0;
        Datos_muestreo.selec_tabla_lectura = 0;
        Datos_muestreo.nro_muestra_en_seg = 0;
        Datos_muestreo.nro_muestra_total_muestreo=0; // Contador de muestras totales en el muestreo.
        Datos_muestreo.flag_tabla_llena = false;
        Datos_muestreo.flag_tomar_muestra = false;
        Datos_muestreo.flag_muestra_perdida = false;
        Datos_muestreo.flag_fin_muestreo = false;
        Datos_muestreo.nro_archivo=0;
        Datos_muestreo.nro_tabla_guardada=0;
        Datos_muestreo.nro_tabla_enviada=0;
        Datos_muestreo.int_contador_segundos=0;
        Datos_muestreo.nro_muestreo=0;
        Datos_muestreo.duracion_muestreo=0;
        Datos_muestreo.cant_muestras_perdidas=0;
        Datos_muestreo.cantidad_de_interrupciones_de_muestreo=0;
        Datos_muestreo.cantidad_de_muestras_leidas=0;


// Apago el LED
        LED=0;
        gpio_set_level(LED_1, 0);


}

void cerrar_archivo(void){

// Cerramos el archivo si está abierto (hay que comprobar que no esté en uso)
        if( xSemaphore_mutex_archivo != NULL ) { //Chequea que el semáforo esté inicializado
                if( xSemaphoreTake( xSemaphore_mutex_archivo, portMAX_DELAY ) == pdTRUE ) {
                        if (f_samples != NULL) {
                                ESP_LOGI(TAG, "CERRANDO ARCHIVO.");
                                fflush(f_samples); // Vacio el buffer
                                fclose(f_samples);
                        }
                        xSemaphoreGive( xSemaphore_mutex_archivo ); // Habilito que otro use el archivo
                }
        } //Chequea que el semáforo de archivo esté inicializado
}
