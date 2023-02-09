# Programa para calibrar un acelerómetro



## Resumen

Este programa desarrolla un calibrador para un acelerómetro, que en principio es el MPU6050. Conectado al puerto I2C del módulo ESP32.
El módulo ESP32 se colocará en un lugar firme, y el acelerómetro sobre una mesa vibratoria.
Se realizan mediciones de 1 minuto, a 500 muestras por segundo.


### Hardware

Se utiliza un módulo ESP32 "NodeMCU-32S"
Un acelerómetro MPU6050
Una Tarjeta de memoria SD (o micro SD)



#### Pines y conexiones:

Pines Tarjeta SD:
Pines Botones y LED:
Pines I2C: (se modifican con el menuconfig)

| MPU6050 Slave    | SDA    | SCL    |
| ---------------- | ------ | ------ |
| ESP32 I2C Master | GPIO21 | GPIO22 |



**Note: ** There’s no need to add an external pull-up resistors for SDA/SCL pin, because the driver will enable the internal pull-up resistors. → PROBADO, FUNCIONA

### Configure the project

idf.py set-target esp32

Open the project configuration menu (`idf.py menuconfig`).

### Build and Flash
IMPORTANTISIMO!!
Con un modulo ESP32 nuevo, programar los fuses

        espefuse.py set_flash_voltage 3.3V      →  Ver problemas booteo

Enter `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)


Para recibir datos del puerto serie en la PC y guardarlos en un archivo:
Ejecutar las siguientes 2 lineas. La primera configura el puerto serie, y la segunda guarda en el archivo.
    stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb
    (stty raw; cat > recibidos.dat) < /dev/ttyUSB0
