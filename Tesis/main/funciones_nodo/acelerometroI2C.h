/** \file	acelerometroI2C.h
 *  \brief	Contiene las funciones de manejo de inicializacion del I2C, lectura y escritura del MPU6050
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo de inicializacion del I2C, lectura y escritura del MPU6050 \
 */

#ifndef ACELEROMETROI2C_H_
#define ACELEROMETROI2C_H_

/*****************************************************************************
* Prototipos
*****************************************************************************/
/** \brief	función que inicializa el puerto I2C
 */
 esp_err_t inicializacion_i2c(void);
 int inicializacion_mpu6050(void);
 esp_err_t escribe_registro(int i2c_slv_address, int reg_num, int valor);
 uint8_t lee_registro(int i2c_slv_address, int reg_num);
 uint8_t lee_mult_registros(int i2c_slv_address, int reg_inicio_num, uint8_t * datos, uint8_t cant_datos);


/*****************************************************************************
* Definiciones
*****************************************************************************/
// Configuraciones del puerto I2C
#define I2C_MASTER_SCL_IO 22                /*!< gpio22 number for I2C master clock */
#define I2C_MASTER_SDA_IO 21                /*!< gpio21 number for I2C master data  */
#define I2C_MASTER_NUM 0                    /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 400000           /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0         /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0         /*!< I2C master doesn't need buffer */


#define ACELEROMETRO_ADDR 0x68   /*!< slave address for BH1750 sensor */
#define REG_107 107
#define REG_1ER_DATO 59
#define CANT_BYTES_LECTURA 14

//#define MPU6050_CMD_START CONFIG_MPU6050_OPMODE   /*!< Operation mode */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */


#endif /* ACELEROMETROI2C_H_ */
