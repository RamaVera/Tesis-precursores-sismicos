/** \file	sd_card.h
 *  \brief	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo e inicializacion de la tarjeta SD
 */
 #ifndef CONFIGURACIONES_H_
 #define CONFIGURACIONES_H_


#define TICTOC_PORT 8080                // Puerto del algoritmo de sincronismo

//#define TICTOC_SERVER "192.168.0.100"   // Ip del servidor de sincronismo
//#define WIFI_SSID "The Dude"        //SSID WIFI (Nombre de red)
//#define WIFI_PASS "zarzaparrilla"   //Contraseña WIFI
//#define IP_BROKER_MQTT "192.168.0.10"   //Direccion del Broker MQTT


#define WIFI_SSID "xxxxxxx"        //SSID WIFI (Nombre de red)
#define WIFI_PASS "xxxxxxx"        //Contraseña WIFI

#define TICTOC_SERVER "192.168.0.50"   // Ip del servidor de sincronismo


// Configuracion del MQTT
#define IP_BROKER_MQTT "192.168.0.100"   //Direccion del Broker MQTT
#define PUERTO_MQTT 1883                //puerto del MQTT
#define USUARIO_MQTT "usr"          //Usuario del MQTT
#define PASSWD_MQTT  "usr"  //Contraseña del MQTT


// #define IP_BROKER_MQTT "192.168.0.100"   //Direccion del Broker MQTT
// #define PUERTO_MQTT 1883                //puerto del MQTT
// #define USUARIO_MQTT "usuario"          //Usuario del MQTT
// #define PASSWD_MQTT  "usuariopassword"  //Contraseña del MQTT


#define ARCHIVOS_CON_ENCABEZADO

//#define SD_40MHZ  // Para mas velocidad en la tarjeta SD (default es 20MHz), a 20MHz es mas estable.

/*
* ------  CONFIGURACIONES DEL MUESTREO  --------
*/
#define CANT_BYTES_LECTURA 14    // Cantidad de bytes leidos del MPU6050 (aceletometro, temperatura, giroscopo)
//#define CANT_BYTES_LECTURA 6    // Cantidad de bytes leidos del MPU6050 (solo acelerometro)
#define MUESTRAS_POR_SEGUNDO 500
#define MUESTRAS_POR_TABLA   500
#define TABLAS_POR_ARCHIVO 60
#define LONG_TABLAS MUESTRAS_POR_TABLA*CANT_BYTES_LECTURA




// Amount of algorithm cycles between estimation parameters computation.
#define P 60 // Sincronización estándar 30 minutos aprox
//#define P 2  //2 minutos para la primera sincronización, la sincronización no sirve (para pruebas nomás)


#endif /* CONFIGURACIONES_H_ */
