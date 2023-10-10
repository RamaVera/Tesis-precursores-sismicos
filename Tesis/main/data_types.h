//
// Created by Ramiro on 6/20/2023.
//

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

typedef struct MPU9250_t {
    uint16_t accelX, accelY, accelZ; /*!< Accelerometer raw data */
} MPU9250_t;

typedef struct MPU9250_time_t {
    MPU9250_t sample;
    int hour;
    int seconds;
    int min;
} MPU9250_time_t;

typedef struct ADC_t{
    uint16_t adcX, adcY, adcZ;
}ADC_t ;

typedef struct ADC_time_t {
    ADC_t sample;
    int hour;
    int seconds;
    int min;
} ADC_time_t;

typedef struct SD_t{
    MPU9250_t mpuData;
    ADC_t adcData;
}SD_t;

typedef struct SD_time_t {
    SD_t sensorsData;
    int hour;
    int seconds;
    int min;
}SD_time_t;



#endif //DATA_TYPES_H
