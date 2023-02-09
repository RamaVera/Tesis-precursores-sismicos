/** \file	tareas.c
 *  \brief	Contiene tareas RTOS del programa
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *
 */

#include "../main.h"
#include "../tictoc/daemon.h"
#include "../tictoc/microtime.h"

#include "mqtt.h"
#include "tareas.h"

#define MENSAJES_MQTT

extern FILE *f_samples;
extern SemaphoreHandle_t xSemaphore_tomamuestra;
extern SemaphoreHandle_t xSemaphore_guardatabla;
extern SemaphoreHandle_t xSemaphore_mutex_archivo;

extern uint8_t LED;

extern muestreo_t Datos_muestreo;
extern mensaje_t mensaje_consola;
extern TicTocData * ticTocData;

static const char *TAG = "TAREAS "; // Para los mensajes del micro

uint32_t muestra_inicial_archivo=0;
bool aux_primer_muestra=true;
uint32_t buffer_cant_interrupciones=0;



void IRAM_ATTR leo_muestras(void *arg)
{
        inicializacion_mpu6050();
        uint8_t cont_pos_escritura;
        uint32_t contador;
//        aux_primer_muestra=true;

        for(contador=0; contador < (CANT_BYTES_LECTURA*MUESTRAS_POR_TABLA); contador++ ) {
                Datos_muestreo.TABLA0[contador] = contador;
                Datos_muestreo.TABLA1[contador] = contador;
        }

        while (1) {

                if( xSemaphore_tomamuestra != NULL )
                {
                        if( xSemaphoreTake( xSemaphore_tomamuestra, portMAX_DELAY) == pdTRUE ) {  // El semaforo se libera a la frecuencia de muestreo
                                buffer_cant_interrupciones = Datos_muestreo.cantidad_de_interrupciones_de_muestreo; // Por si cambia mientras estoy procesando la muestra

// Simulación de pérdida de muestras/////
                                if(0==gpio_get_level(BOTON_1)) { // si presiono el boton
                                        vTaskDelay(100); // Este delay me hace perder muestras
                                }
////////////////////////////////////////

                                if (LED == 0) {
                                        gpio_set_level(LED_1, 1);
                                        LED=1;
                                }
                                else {
                                        LED=0;
                                        gpio_set_level(LED_1, 0);
                                }

                                if(Datos_muestreo.nro_muestra_en_seg==0 && Datos_muestreo.nro_tabla_enviada == 0 && aux_primer_muestra == true) { // Si es la primer muestra del archivo
                                        muestra_inicial_archivo = buffer_cant_interrupciones;
                                        aux_primer_muestra=false;

                                        // sprintf(mensaje_consola.mensaje,"Muestra_inicial_archivo: %u", muestra_inicial_archivo);
                                        // mensaje_consola.mensaje_nuevo=true;
                                }

                                lee_mult_registros(ACELEROMETRO_ADDR, REG_1ER_DATO, Datos_muestreo.datos_mpu, CANT_BYTES_LECTURA); // Leo todos los registros de muestreo del acelerometro/giroscopo
                                Datos_muestreo.cantidad_de_muestras_leidas++;
                                Datos_muestreo.flag_tomar_muestra = false; //AVISO QUE LA MUESTRA FUE LEIDA

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// COMPROBAMOS SI SE PERDIERON MUESTRAS Y RELLENAMOS CON CEROS LAS MUESTRAS PERDIDAS EN LA TABLA ///////////////////////////////

                                uint32_t cant_muestras_perdidas=0;
                                cant_muestras_perdidas = 0;

                                if(buffer_cant_interrupciones > Datos_muestreo.cantidad_de_muestras_leidas) { // Si hay muestras perdidas tengo que rellenar con ceros

                                        cant_muestras_perdidas = (buffer_cant_interrupciones - Datos_muestreo.cantidad_de_muestras_leidas)+1;  // Cantidad de muestras faltantes, agrego una por la muestra que estoy descartando
                                        uint32_t aux_leidas=Datos_muestreo.cantidad_de_muestras_leidas;
                                        Datos_muestreo.cantidad_de_muestras_leidas--; // Descarto la muestra leida para este muestreo y relleno con ceros

                                        // sprintf(mensaje_consola.mensaje,"MUESTRAS PERDIDAS, cantidad: %u  | Interruciones: %u  | Muestras Leidas: %u \n",
                                        //         cant_muestras_perdidas, Datos_muestreo.cantidad_de_interrupciones_de_muestreo, Datos_muestreo.cantidad_de_muestras_leidas);
                                        // mensaje_consola.mensaje_nuevo=true;

                                        // ESP_LOGI(TAG, "MUESTRAS PERDIDAS, cantidad: %u  | Interruciones: %u  | Muestras Leidas: %u",
                                        //         cant_muestras_perdidas, Datos_muestreo.cantidad_de_interrupciones_de_muestreo, Datos_muestreo.cantidad_de_muestras_leidas);

                                        uint32_t aux_muestras_relleno = 0;  // Cantidad de muestras en cero que voy a guardar
                                        if(cant_muestras_perdidas > MUESTRAS_POR_TABLA-Datos_muestreo.nro_muestra_en_seg ) { // Si tengo demasiadas muestras perdidas y no entran en la tabla, entonces lo truncamos
                                                aux_muestras_relleno = MUESTRAS_POR_TABLA-Datos_muestreo.nro_muestra_en_seg; //Cantidad de muestras que puedo agregar en esta tabla.
                                                ESP_LOGI(TAG, "TRUNCAMIENTO: Cantidad de muestras perdidas: %u | Cantidad truncada: %u | Nro muestra en seg: %u \n",cant_muestras_perdidas,  aux_muestras_relleno, Datos_muestreo.nro_muestra_en_seg);
                                        }
                                        else{
                                                aux_muestras_relleno = cant_muestras_perdidas;
                                        }

                                        // Datos_muestreo.cantidad_de_muestras_leidas--; // Resto una porque había sumado una de más antes

                                        //aux_muestras_relleno++;
                                        while (aux_muestras_relleno > 0 ) {

                                                if (Datos_muestreo.selec_tabla_escritura == 0) {
                                                        for (cont_pos_escritura = 0; cont_pos_escritura < CANT_BYTES_LECTURA; cont_pos_escritura++ ) {
                                                                Datos_muestreo.TABLA0[cont_pos_escritura + (CANT_BYTES_LECTURA * Datos_muestreo.nro_muestra_en_seg)] = 0; // RELLENO CON UNA MUESTRA DE CEROS
                                                        }
                                                }
                                                else {
                                                        for (cont_pos_escritura = 0; cont_pos_escritura < CANT_BYTES_LECTURA; cont_pos_escritura++ ) {
                                                                Datos_muestreo.TABLA1[cont_pos_escritura + (CANT_BYTES_LECTURA * Datos_muestreo.nro_muestra_en_seg)] = 0; // RELLENO CON UNA MUESTRA DE CEROS
                                                        }
                                                }

                                                Datos_muestreo.cantidad_de_muestras_leidas++;   // Porque agregué una muestra de ceros.
                                                Datos_muestreo.nro_muestra_en_seg++;            // Porque agregué una muestra de ceros.
                                                aux_muestras_relleno--;
                                        }

                                        sprintf(mensaje_consola.mensaje,"MP: Cant_inic: %u Post: %u Leidas_inic %d Leidas_pos %d INT: %d ",
                                                cant_muestras_perdidas,
                                                buffer_cant_interrupciones - Datos_muestreo.cantidad_de_muestras_leidas,
                                                aux_leidas,
                                                Datos_muestreo.cantidad_de_muestras_leidas,
                                                buffer_cant_interrupciones);

                                        mensaje_consola.mensaje_nuevo=true;

                                }
                                else{

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SI NO TUVIMOS MUESTRAS PERDIDAS GUARDAMOS LA MUESTRA LEIDA DEL ACELERÓMETRO /////////////////////////////////////////////////////////////////////////////////

                                        if(Datos_muestreo.nro_muestra_en_seg < MUESTRAS_POR_TABLA) {     // COMPRUEBO SI QUEDA LUGAR EN LA TABLA PARA GUARDAR LA MUESTRA, SINÓ NO LA GUARDO
                                                if (Datos_muestreo.selec_tabla_escritura == 0) {
                                                        for (cont_pos_escritura = 0; cont_pos_escritura < CANT_BYTES_LECTURA; cont_pos_escritura++ ) {
                                                                Datos_muestreo.TABLA0[cont_pos_escritura + (CANT_BYTES_LECTURA * Datos_muestreo.nro_muestra_en_seg)] = Datos_muestreo.datos_mpu [cont_pos_escritura];                                                             // Agrega los bytes leidos a la tabla 0
                                                        }
                                                }
                                                else {
                                                        for (cont_pos_escritura = 0; cont_pos_escritura < CANT_BYTES_LECTURA; cont_pos_escritura++ ) {
                                                                Datos_muestreo.TABLA1[cont_pos_escritura + (CANT_BYTES_LECTURA * Datos_muestreo.nro_muestra_en_seg)] = Datos_muestreo.datos_mpu [cont_pos_escritura];                                                             // Agrega los bytes leidos a la tabla 1
                                                        }
                                                }
                                                Datos_muestreo.nro_muestra_en_seg++;
                                        }

                                }





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DETECTAMOS SI SE LLENÓ LA TABLA  ////////////////////////////////////////////////////////////////////////////////////////////

                                if (Datos_muestreo.nro_muestra_en_seg >= MUESTRAS_POR_TABLA) { // Si llené una tabla paso a la siguiente, y habilito su almacenamiento.

                                        if (Datos_muestreo.flag_tabla_llena == true) { // Si es true es porque no se grabó la tabla anterior
                                                Datos_muestreo.flag_tabla_perdida = true; // Para dar aviso de la pérdida de información.
                                        }

                                        if (Datos_muestreo.selec_tabla_escritura == 0) {
                                                Datos_muestreo.selec_tabla_escritura = 1; // Paso a escribir en la otra tabla
                                                Datos_muestreo.selec_tabla_lectura = 0; // Indico lectura en la tabla recientemente llena
                                        }
                                        else if (Datos_muestreo.selec_tabla_escritura == 1) {
                                                Datos_muestreo.selec_tabla_escritura = 0; // Paso a escribir en la otra tabla
                                                Datos_muestreo.selec_tabla_lectura = 1; // Indico lectura en la tabla recientemente llena
                                        }
                                        Datos_muestreo.flag_tabla_llena = true;
                                        Datos_muestreo.nro_muestra_en_seg = 0; // Reinicio el contador de muestras

                                        Datos_muestreo.nro_tabla_enviada++; // Envio una nueva tabla para guardar

                                        if (Datos_muestreo.nro_tabla_enviada >= TABLAS_POR_ARCHIVO) {
                                                Datos_muestreo.nro_tabla_enviada=0; // Envio una nueva tabla para guardar
                                        }
                                        xSemaphoreGive( xSemaphore_guardatabla ); // Habilito la escritura de la tabla
                                }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        } // Toma semaforo
                } // comprueba existencia semaforos
        } // while (1)
        vTaskDelete(NULL);
}

void IRAM_ATTR guarda_datos(void *arg)
{
        char archivo[40]; // Para guardar el nombre del archivo

        while (1) {
                if( xSemaphore_guardatabla != NULL ) { //Chequea que el semáforo esté inicializado
                        if( xSemaphoreTake( xSemaphore_guardatabla, portMAX_DELAY ) == pdTRUE ) //Si se guardó una tabla de datos se libera el semáforo
                        {
                                if( xSemaphore_mutex_archivo != NULL ) { //Chequea que el semáforo esté inicializado
                                        if( xSemaphoreTake( xSemaphore_mutex_archivo, portMAX_DELAY ) == pdTRUE ) {

                                                Datos_muestreo.nro_tabla_guardada++; // Aumento contador de tablas llenas

                                                // if (LED == 0) {
                                                //         gpio_set_level(LED_1, 1);
                                                //         LED=1;
                                                // }
                                                // else {
                                                //         LED=0;
                                                //         gpio_set_level(LED_1, 0);
                                                // }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DETECTAMOS SI HAY QUE EMPEZAR UN ARCHIVO NUEVO, LO CREAMOS Y LO ABRIMOS /////////////////////////////////////////////////////

                                                if (Datos_muestreo.nro_tabla_guardada == 1) { // Inicio un archivo nuevo

                                                        sprintf(archivo, MOUNT_POINT "/%d-%d.dat", Datos_muestreo.nro_muestreo, Datos_muestreo.nro_archivo );
                                                        Datos_muestreo.nro_archivo++; // El proxímo archivo tendrá otro número

                                                        f_samples = fopen(archivo, "w"); // Abro un archivo nuevo

                                                        if (f_samples == NULL) {
                                                                ESP_LOGE(TAG, "Falló al abrir el archivo para escribir");
                                                        }
                                                        else {

              #ifdef ARCHIVOS_CON_ENCABEZADO
//
                                                                fprintf(f_samples,"%010u\n",muestra_inicial_archivo);
                                                                fprintf(f_samples,"%010d\n",MUESTRAS_POR_SEGUNDO);


                                                                aux_primer_muestra=true;

                                                                ESP_LOGI(TAG, "Nuevo archivo. %s  - muestra_inicial_archivo= %u Datos_muestreo.cantidad_de_interrupciones_de_muestreo= %u", archivo, muestra_inicial_archivo, buffer_cant_interrupciones );
              #endif
                                                        }
                                                }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GUARDAMOS LA TABLA ACTIVA EN EL ARCHIVO ABIERTO, SE SE LLENA LO CERRAMOS  ///////////////////////////////////////////////////

                                                if (f_samples == NULL) {
                                                        ESP_LOGE(TAG, "EL ARCHIVO NO ESTA ABIERTO. ERROR EN LA TARJETA");
                                                }
                                                else{
                                                        // GUARDO LAS TABLAS
                                                        if (Datos_muestreo.selec_tabla_lectura == 0) {
                                                                fwrite(Datos_muestreo.TABLA0, sizeof(uint8_t), sizeof(Datos_muestreo.TABLA0), f_samples);
                                                        }
                                                        else {
                                                                fwrite(Datos_muestreo.TABLA1, sizeof(uint8_t), sizeof(Datos_muestreo.TABLA1), f_samples);
                                                        }
                                                        fflush(f_samples); // Vacio el buffer

                                                        Datos_muestreo.flag_tabla_llena = false; // Aviso que guardé la tabla, para detectar errores

                                                        if (Datos_muestreo.nro_tabla_guardada >= TABLAS_POR_ARCHIVO) { // Porque grabé en el archivo todas las tablas que tenia que guardar
                                                                fclose(f_samples);
                                                                ESP_LOGI(TAG, "Archivo guardado, numeroDeTablas: %u",Datos_muestreo.nro_tabla_guardada);
                                                                Datos_muestreo.nro_tabla_guardada = 0; // Reinicio el contador de tablas en archivo
                                                        }
                                                }

                                                xSemaphoreGive( xSemaphore_mutex_archivo ); // Habilito que otro use el archivo
                                        }
                                } //Chequea que el semáforo de archivo esté inicializado
                        }
                }
        } //while(1)
        vTaskDelete(NULL);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TAREA QUE ENVÍA MENSAJES SIN BLOQUEAR   /////////////////////////////////////////////////////////////////////////////////////

void muestra_info(void *arg)
{
        while(1) {


                if (mensaje_consola.mensaje_nuevo == true) {
                        mensaje_consola.mensaje_nuevo=false;
                        //printf(mensaje_consola.mensaje);
                        ESP_LOGI(TAG, "%s", mensaje_consola.mensaje );
                }

                if (Datos_muestreo.flag_fin_muestreo == true) {
                        ESP_LOGI(TAG, "Enviando mendaje MQTT de fin de muestreo");
                        Datos_muestreo.flag_fin_muestreo = false;
                        mensaje_fin_muestreo(Datos_muestreo.nro_muestreo);
                }


                if (Datos_muestreo.flag_muestra_perdida == true) {
                        ESP_LOGE(TAG, "Muestra perdida.");
                        Datos_muestreo.flag_muestra_perdida = false;
                        mensaje_mqtt_error("Muestra perdida");
                }

                if (Datos_muestreo.flag_tabla_perdida == true) {
                        ESP_LOGE(TAG, "Tabla perdida");
                        Datos_muestreo.flag_tabla_perdida = false;
                        mensaje_mqtt_error("Tabla perdida");
                }


                // struct timeval current_time2;
                // gettimeofday(&current_time2, NULL);
                // uint32_t epoch_test=current_time2.tv_sec;
                //
                // ESP_LOGI(TAG, "Hora leida: %d", epoch_test);
                //
                //
                // int64_t tt12 = esp_timer_get_time();
                // ESP_LOGI(TAG, "Hora get_time: %lld", tt12);
                //

//            ESP_LOGI(TAG, "DEBUG | Int= %u | Muestras= %u | Nro en seg= %u | Tabla= %d",buffer_cant_interrupciones, Datos_muestreo.cantidad_de_muestras_leidas, Datos_muestreo.nro_muestra_en_seg, Datos_muestreo.nro_tabla_guardada);


//
// #ifdef MUESTRA_ESTADISTICAS_CPU
//                 static char cBuffer[ 512 ];
//                 vTaskGetRunTimeStats( cBuffer );
//                 printf("Estadisticas: \n");
//                 printf(cBuffer);
// #endif

                // printf("Duracion muestreo = %d \n",Datos_muestreo.duracion_muestreo);
                // printf("Contador de segundos = %d \n",Datos_muestreo.contador_segundos);


                vTaskDelay(500 / portTICK_PERIOD_MS);

        }
        vTaskDelete(NULL); // Nunca se va a ejecutar
}
