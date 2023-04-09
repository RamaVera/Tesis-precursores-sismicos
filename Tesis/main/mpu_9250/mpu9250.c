/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#include "mpu9250.h"

static const char *TAG = "MPU9250"; // Para los mensajes de LOG

spi_device_handle_t spi_mpu;

esp_err_t MPU9250_init(void) {
    esp_err_t ret;
    spi_bus_config_t buscfg = {
            .miso_io_num = MPU_PIN_NUM_MISO_ADO,
            .mosi_io_num = MPU_PIN_NUM_MOSI_SDA,
            .sclk_io_num = MPU_PIN_NUM_CLK_SCL,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4092
    };
    spi_device_interface_config_t devcfg = {
            .command_bits = 8,
            .address_bits = 0,
            .dummy_bits = 0,
            .clock_speed_hz = 400000,                 //Clock out at 400KHz
            .mode = 0,                                //SPI mode 0
            .spics_io_num = MPU_PIN_NUM_CS,               //CS pin
            .queue_size = 1,                          //queue size
            .flags = 0,
    };
    //Initialize the SPI bus
    // Sep/24/2017 SPI DMA gives corrupted data on MPU-9250 fifo access
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, MPU_SPI_DMA_CHAN);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "MPU9250: Invalid init");
        return ESP_FAIL;
    }
    //Attach the slave devices to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi_mpu);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "MPU9250: Invalid attach");
        return ESP_FAIL;
    }


    uint8_t rv;
    rv = mpu9250_read(WHO_IM_I);
    ESP_LOGI(TAG, "MPU9250: retrieved id: %02x\n", rv);
    if ( rv != MPU9250_ID  && rv != MPU92XX_ID) {
        ESP_LOGE(TAG, "MPU9250: Wrong id: %02x\n", rv);
        return ESP_FAIL;
    }

    ret = MPU9250_reset();
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "MPU9250: Invalid reset");
        return ESP_FAIL;
    }

    ret = mpu9250_start();
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "MPU9250: Invalid start");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t MPU9250_enableInterruptWith(gpio_isr_t functionToDoWhenRiseAnInterrupt) {

    // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared, clear on read of INT_STATUS
    if( mpu9250_write(INT_PIN_CFG, 0x20)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    gpio_pad_select_gpio(MPU_PIN_NUM_INT);
    gpio_set_direction(MPU_PIN_NUM_INT, GPIO_MODE_INPUT);
    gpio_pulldown_en(MPU_PIN_NUM_INT);
    gpio_pullup_dis(MPU_PIN_NUM_INT);
    gpio_set_intr_type(MPU_PIN_NUM_INT, GPIO_INTR_POSEDGE);

    gpio_install_isr_service(0);
    if( gpio_isr_handler_add(MPU_PIN_NUM_INT, functionToDoWhenRiseAnInterrupt, NULL ) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    if(MPU9250_enableInterrupt(false) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1 / portTICK_PERIOD_MS);

    return ESP_OK;

}

esp_err_t MPU9250_enableInterrupt(bool enable) {
    // INT enable on RDY
    return mpu9250_write(INT_ENABLE, enable?0x01:0x00);
}

esp_err_t MPU9250_reset()
{
    // Write a one to bit 7 reset bit; toggle reset device
    if( mpu9250_write(PWR_MGMT_1, 0x80) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Auto select clock source to be PLL gyroscope
    if( mpu9250_write(PWR_MGMT_1, 0x01) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    return ESP_OK;
}

esp_err_t mpu9250_start(void)
{
    // Enable Gyroscope and Accelerometer
    //if( mpu9250_write(PWR_MGMT_2, 0x00)          != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Disable Gyroscope
    if( mpu9250_write(PWR_MGMT_2, 0x07)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // FCHOICE [1 1] DLPF_CFG [0] Gyroscope--> 250Hz Delay:0.97  FS:8 Khz
    if( mpu9250_write(MPU_CONFIG, 0x06)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // FCHOICEB [0 0] Gyro 2000dps
    if( mpu9250_write(GYRO_CONFIG, 0x18)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Accel scale 2g
    if( mpu9250_write(ACCEL_CONFIG, 0x08)   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // FCHOICE [1] A_DLPF_CFG 0x02 3dB BW:9Hz Fs:1 KHz DLPF Delay:2.88
    if( mpu9250_write(ACCEL_CONFIG2, 0x02)  != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Set sample rate = gyroscope output rate 8000Khz/(1 + SMPLRT_DIV) ---> 1000Khz / (1+0) = 1000Khz
    if( mpu9250_write(SMPLRT_DIV, 0x00)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Power down magnetometer
    if( mpu9250_write(AK8963_CNTL1, 0x00)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    return  ESP_OK;

}

uint8_t mpu9250_read(uint8_t reg){
    esp_err_t ret;
    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.length = 8;
    trans.rxlength = 8;
    trans.cmd = reg | 0x80;
    trans.flags = SPI_TRANS_USE_RXDATA;
    ret = spi_device_transmit(spi_mpu, &trans);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "MPU9250: Invalid read %d", ret);
    }
    assert(ret == ESP_OK);

    return trans.rx_data[0];
}

esp_err_t mpu9250_write(uint8_t reg, uint8_t val)
{
    esp_err_t ret;
    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.length = 8;
    trans.cmd = reg & 0x7f;
    trans.tx_data[0] = val;
    trans.flags = SPI_TRANS_USE_TXDATA;
    //printf("do transfer\n");
    ret = spi_device_transmit(spi_mpu, &trans);
    return ret;
}

esp_err_t mpu9250_readn(uint8_t reg, uint8_t *buf, size_t len)
{
    esp_err_t ret;
    spi_transaction_t trans;
    uint8_t *rbuf = heap_caps_malloc(len, MALLOC_CAP_DMA);
    if (rbuf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.cmd = reg | 0x80;
    trans.length = 8*len;
    trans.rxlength = 8*len;
    trans.rx_buffer = rbuf;
    //printf("do transfer\n");
    //Queue all transactions.
    ret = spi_device_transmit(spi_mpu, &trans);
    if (ret != ESP_OK) {
        free(rbuf);
        return ret;
    }
    memcpy(buf, rbuf, len);
    free(rbuf);
    return ret;
}

bool mpu9250_ready(void)
{
    uint8_t val = mpu9250_read(INT_STATUS);
    return (val & 1);
}


int mpu9250_fifo_count(void)
{
    uint8_t c[2];
    esp_err_t rv;
    rv = mpu9250_readn(FIFO_COUNTH, c, 2);
    return (rv == ESP_OK) ? be16_val(c, 0) : 0;
}

bool mpu9250_read_fifo(struct sample *rx)
{
    esp_err_t rv;
    rv = mpu9250_readn(FIFO_R_W, (uint8_t *)rx, sizeof(struct sample));
    return (rv == ESP_OK);
}

bool check_fifo(int t)
{
    static int raw;
    static bool cached = false;
    if (cached && t - raw > -340 && t - raw < 340) {
        return true;
    }
    uint8_t temp[2];
    if (ESP_OK == mpu9250_readn(TEMP_OUT_H, temp, 2)) {
        raw = be16_val(temp, 0);
        cached = true;
    }
    return (t - raw > -340 && t - raw < 340);
}

void mpu9250_fifo_reset(void)
{
    uint8_t val = mpu9250_read(USER_CTRL);
    val &= ~0x44;
    mpu9250_write(FIFO_EN, 0);
    mpu9250_write(USER_CTRL, val);
    mpu9250_write(USER_CTRL, val|0x04);
    mpu9250_write(USER_CTRL, val|0x40);
    // All except external sensors
    mpu9250_write(FIFO_EN, 0xf8);
    vTaskDelay(1/portTICK_PERIOD_MS);
}

#if 0
static bool mpu9250_read_sample(struct sample *rx)
{
    esp_err_t rv;
    rv = mpu9250_readn(ACCEL_XOUT_H, (uint8_t *)rx, sizeof(struct sample));
    return (rv == ESP_OK);
}
#endif

__attribute__((unused))  void slv0_readn(uint8_t reg, uint8_t size)
{
    mpu9250_write(I2C_SLV0_CTRL, 0);
    mpu9250_write(I2C_SLV0_ADDR, 0x80 | AK8963_I2C_ADDR);
    mpu9250_write(I2C_SLV0_REG, reg);
    mpu9250_write(I2C_SLV0_CTRL, 0x80 | size);
}

__attribute__((unused))  void slv0_write1(uint8_t reg, uint8_t out)
{
    mpu9250_write(I2C_SLV0_CTRL, 0);
    mpu9250_write(I2C_SLV0_DO, out);
    mpu9250_write(I2C_SLV0_ADDR, AK8963_I2C_ADDR);
    mpu9250_write(I2C_SLV0_REG, reg);
    mpu9250_write(I2C_SLV0_CTRL, 0x80 | 1);
}

static struct ak_asa ak8963_asa;
static float ak8963_calib[3];

__attribute__((unused)) void ak8963_start(void)
{
    // Reset
    // ak8963_write(AK8963_CNTL2, 0x01);

    // Calibrate - fuse, 16-bit adc
    ak8963_write(AK8963_CNTL1, 0x1f);
    ak8963_read_asa(&ak8963_asa);

    for (int i = 0; i < 3; i++) {
        float data = ak8963_asa.a[i];
        // factory sensitivity
        ak8963_calib[i] = ((data - 128) / 256 + 1);
        // adjust by ADC sensitivity and convert to milligauss
        ak8963_calib[i] *= ADC_16BIT_RESOLUTION * AK8963_MILLIGAUSS_SCALE;
    }

    // Setup mode - continuous mode 2, 16-bit adc
    ak8963_write(AK8963_CNTL1, 0x16);
    // Start measurement
}

__attribute__((unused)) uint8_t ak8963_read(uint8_t reg)
{
    slv0_readn(reg, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    uint8_t rv = mpu9250_read(EXT_SENS_DATA_00);

    mpu9250_write(I2C_SLV0_CTRL, 0);
    return rv;
}

__attribute__((unused)) void ak8963_write(uint8_t reg, uint8_t val)
{
    slv0_write1(reg, val);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    mpu9250_write(I2C_SLV0_CTRL, 0);
}

__attribute__((unused)) void ak8963_read_sample_start(void)
{
    slv0_readn(AK8963_HXL, 7);
}

__attribute__((unused)) bool ak8963_read_sample(struct ak_sample *rx)
{
    esp_err_t rv;
    rv = mpu9250_readn(EXT_SENS_DATA_00, (uint8_t *)rx,
                       sizeof(struct ak_sample));

    mpu9250_write(I2C_SLV0_CTRL, 0);
    return (rv == ESP_OK);
}

__attribute__((unused)) bool ak8963_read_asa(struct ak_asa *rx)
{
    slv0_readn(AK8963_ASAX, 3);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    esp_err_t rv;
    rv = mpu9250_readn(EXT_SENS_DATA_00, (uint8_t *)rx, sizeof(struct ak_asa));
    if (rv != ESP_OK) {
        return false;
    }

    mpu9250_write(I2C_SLV0_CTRL, 0);
    return true;
}


//if (ret == ESP_OK) {
//ESP_LOGI(TAG, "READ OK");
//} else if (ret == ESP_ERR_TIMEOUT) {
//ESP_LOGW(TAG, "Bus is busy");
//} else if (ret == ESP_FAIL) {
//ESP_LOGW(TAG, "ACK no recibido");
//} else {
//ESP_LOGE(TAG, "Read single Failed");
//}