//
// Created by Ramiro on 6/20/2023.
//

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

typedef struct MPU9250_t {
    uint8_t AxH, AyH, AzH;         /*!< Accelerometer raw data */
    uint8_t AxL, AyL, AzL;         /*!< Accelerometer raw data */
} MPU9250_t;

typedef struct ADC_t{
    int data;
}ADC_t ;

typedef struct SD_sensors_data{
    MPU9250_t mpuData;
    ADC_t adcData;
}SD_sensors_data_t;

typedef struct SD_data {
    SD_sensors_data_t sensorsData;
    int hour;
    int seconds;
    int min;
}SD_data_t;



#endif //DATA_TYPES_H
