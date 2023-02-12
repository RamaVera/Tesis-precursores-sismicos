/** \file	acelerometroI2C.h
 *  \brief	Contiene las funciones de manejo de inicializacion del I2C, lectura y escritura del MPU6050
 *  Autor: Ramiro Alonso
 *  Versión: 1
 *	Contiene las funciones de manejo de inicializacion del I2C, lectura y escritura del MPU6050 \
 */

#include <stdio.h>
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "acelerometroI2C.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "MPU6050 ";

///////////////////////////////////////////////////////////////////////////////

/************************************************************************
* Variables
************************************************************************/
static int i2c_master_port;

// Lugar donde guardo los datos leidos del mpu
char uch_Datos_mpu[14];

/**
 * @brief Inicialización del puerto i2c
 */
esp_err_t inicializacion_i2c(void)
{

    i2c_master_port = I2C_MASTER_NUM;


    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

//     i2c_config_t conf;
//     conf.mode = I2C_MODE_MASTER;
//     conf.sda_io_num = I2C_MASTER_SDA_IO;
//     conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
// //    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
//     conf.scl_io_num = I2C_MASTER_SCL_IO;
//     conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
// //    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
//     conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);

//    vTaskDelay(30 / portTICK_RATE_MS);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief Configuraciones iniciales del acelerometro MPU6050
 */

int inicializacion_mpu6050(void)
{
  escribe_registro(ACELEROMETRO_ADDR, REG_107, 0x00);
    return 0;
}

/**
 * @brief Escritura de un registro del acelerometro MPU6050
 */

esp_err_t escribe_registro(int i2c_slv_address, int reg_num, int valor)
{
  int ret=0;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, i2c_slv_address << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg_num, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, valor, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);

  if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Write OK");
  } else if (ret == ESP_ERR_TIMEOUT) {
      ESP_LOGW(TAG, "Bus is busy");
  } else if (ret == ESP_FAIL) {
      ESP_LOGW(TAG, "ACK no recibido");
  } else {
      ESP_LOGE(TAG, "Write Failed");
  }
    return ret;
}

// /**
//  * @brief Write a byte to a MPU9250 sensor register
//  */
// static esp_err_t escribe_registro(int i2c_slv_address, uint8_t reg_addr, uint8_t data)
// {
//     int ret;
//     uint8_t write_buf[2] = {reg_addr, data};
//
//     ret = i2c_master_write_to_device(I2C_MASTER_NUM, i2c_slv_address, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
//
//     return ret;
// }



/**
 * @brief Lectura de un registro del acelerometro MPU6050, devuelve el valor leido
 */

uint8_t IRAM_ATTR lee_registro(int i2c_slv_address, int reg_num)
{
      uint8_t valor=0;
      int32_t ret=0;
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, i2c_slv_address << 1 | WRITE_BIT, ACK_CHECK_EN);
      i2c_master_write_byte(cmd, reg_num, ACK_CHECK_EN);
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, i2c_slv_address << 1 | READ_BIT, ACK_CHECK_EN);
      i2c_master_read_byte(cmd, &valor, NACK_VAL);
      i2c_master_stop(cmd);
      ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS); // Ejecuto el comando
      i2c_cmd_link_delete(cmd);  // Borro el comando

      if (ret == ESP_OK) {
          ESP_LOGI(TAG, "READ OK");
      } else if (ret == ESP_ERR_TIMEOUT) {
          ESP_LOGW(TAG, "Bus is busy");
      } else if (ret == ESP_FAIL) {
          ESP_LOGW(TAG, "ACK no recibido");
      } else {
          ESP_LOGE(TAG, "Read single Failed");
      }
    return valor;
}

uint8_t IRAM_ATTR lee_mult_registros(int i2c_slv_address, int reg_inicio_num, uint8_t * datos, uint8_t cant_datos)
{
      int32_t ret=0;
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, i2c_slv_address << 1 | WRITE_BIT, ACK_CHECK_EN);
      i2c_master_write_byte(cmd, reg_inicio_num, ACK_CHECK_EN);
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, i2c_slv_address << 1 | READ_BIT, ACK_CHECK_EN);
      if (cant_datos > 1) {
          i2c_master_read(cmd, datos, cant_datos - 1, ACK_VAL);
      }
      i2c_master_read_byte(cmd, datos + cant_datos - 1, NACK_VAL);

      i2c_master_stop(cmd);
      ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_RATE_MS);
      i2c_cmd_link_delete(cmd);

      if (ret == ESP_OK) {
          ESP_LOGI(TAG, "READ OK");
      } else if (ret == ESP_ERR_TIMEOUT) {
          ESP_LOGW(TAG, "Bus is busy");
      } else if (ret == ESP_FAIL) {
          ESP_LOGW(TAG, "ACK no recibido");
      } else {
          ESP_LOGE(TAG, "Read multiple Failed");
      }

    return ret;
}
