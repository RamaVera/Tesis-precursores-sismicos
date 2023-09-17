/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#include "mpu9250.h"

static const char *TAG = "MPU9250"; // Para los mensajes de LOG

//static MPU9250_t Offset = {0,0,0};

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
            .spics_io_num = MPU_PIN_NUM_CS,           //CS pin
            .queue_size = 1,                          //queue size
            .flags = 0,
            .pre_cb = NULL,
            .post_cb = NULL,
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
    ESP_LOGI(TAG, "MPU9250: retrieved id: %02x", rv);
    if ( rv != MPU9250_ID  && rv != MPU92XX_ID) {
        ESP_LOGE(TAG, "MPU9250: Wrong id: %02x", rv);
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

esp_err_t MPU9250_attachInterruptWith(gpio_isr_t functionToDoWhenRiseAnInterrupt, bool enableInterrupt) {

    // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared, clear on read of INT_STATUS
    if( mpu9250_write(INT_PIN_CFG, 0x20)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    gpio_pad_select_gpio(MPU_PIN_NUM_INT);
    gpio_set_direction(MPU_PIN_NUM_INT, GPIO_MODE_INPUT);
    gpio_pulldown_en(MPU_PIN_NUM_INT);
    gpio_pullup_dis(MPU_PIN_NUM_INT);
    gpio_set_intr_type(MPU_PIN_NUM_INT, GPIO_INTR_POSEDGE);

    gpio_install_isr_service(0);
    if( gpio_isr_handler_add(MPU_PIN_NUM_INT, functionToDoWhenRiseAnInterrupt, NULL ) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    if(MPU9250_enableInterrupt(enableInterrupt) != ESP_OK){ return ESP_FAIL;} vTaskDelay(1 / portTICK_PERIOD_MS);

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
    // Set sample rate = gyroscope output rate 8000Khz/(1 + SMPLRT_DIV) ---> 1000hz / (1+0) = 1000hz
    if( mpu9250_write(SMPLRT_DIV, 0x00)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    // Power down magnetometer
    if( mpu9250_write(AK8963_CNTL1, 0x00)     != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    return  ESP_OK;

}

esp_err_t MPU9250_ReadAcce(MPU9250_t * sampleOfMPU) {
    uint8_t data[6];

    if( mpu9250_readn(ACCEL_XOUT_H,data,6)  != ESP_OK){
        return ESP_FAIL;
    }

    sampleOfMPU->accelX = ((int16_t)data[0] << 8) | data[1];
    sampleOfMPU->accelY = ((int16_t)data[2] << 8) | data[3];
    sampleOfMPU->accelZ = ((int16_t)data[4] << 8) | data[5];
//
//    float AMult = 2.0f / 32768.0f;
//
//    sampleOfMPU->Ax = (float)axRaw * AMult - Offset.Ax;
//    sampleOfMPU->Ay = (float)ayRaw * AMult - Offset.Ay;
//    sampleOfMPU->Az = (float)azRaw * AMult - Offset.Az;
    return ESP_OK;
}


esp_err_t MPU9250_SetCalibrationForAccel(MPU9250_t * meanAccel){

//    Offset.Ax = meanAccel->Ax;
//    Offset.Ay = meanAccel->Ay;
//    Offset.Az = meanAccel->Az + ((meanAccel->Az > 0L)? -1.0:1.0);

    /*
    float AMult = 2.0f / 32768.0f;

    // Remove gravity from the z-axis accelerometer bias calculation

    int32_t accel_bias[3] = {
            (int32_t) (meanAccel->Ax)/AMult ,
            (int32_t) (meanAccel->Ay)/AMult ,
            (int32_t) ((meanAccel->Az + ((meanAccel->Az > 0L)? -1.0:1.0))/AMult)};

    ESP_LOGI(TAG,"Calculated offset to apply %x, %x, %x",accel_bias[0],accel_bias[1],accel_bias[2]);

    int16_t accel_bias_reg[3] = { 0, 0, 0 }; // A place to hold the factory accelerometer trim biases
    int16_t new_accel_bias_reg[3] = { 0, 0, 0 };
    int16_t mask_bit[3] = { 1, 1, 1 };// Define array to hold mask bit for each accelerometer bias axis
    uint8_t data[6]; // data array to hold accelerometer and gyro x, y, z, data

    // Read factory accelerometer trim values
    if( mpu9250_readn(XA_OFFSET_H,&data[0],2)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_readn(YA_OFFSET_H,&data[2],2)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_readn(ZA_OFFSET_H,&data[4],2)    != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    ESP_LOGI(TAG,"Previous Offset is %02x%02x, %02x%02x, %02x%02x",data[0],data[1],data[2],data[3],data[4],data[5]);

    accel_bias_reg[0] = ((int16_t) data[0] << 8) | data[1];
    accel_bias_reg[1] = ((int16_t) data[2] << 8) | data[3];
    accel_bias_reg[2] = ((int16_t) data[4] << 8) | data[5];
    ESP_LOGI(TAG,"Previous Offset as accel_bias_reg %x, %x, %x",accel_bias_reg[0],accel_bias_reg[1],accel_bias_reg[2]);
    ESP_LOGI(TAG,"Previous Offset as float is %02f, %02f, %02f",(float)accel_bias_reg[0] * AMult,(float)accel_bias_reg[1]  * AMult,(float)accel_bias_reg[2] * AMult);


    for (int i = 0; i < 3; i++) {
        if (accel_bias_reg[i] % 2) {
            mask_bit[i] = 0;
        }
        accel_bias_reg[i] -= accel_bias[i]; // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g
        if (mask_bit[i]) {
            accel_bias_reg[i] = accel_bias_reg[i] & ~mask_bit[i]; // Preserve temperature compensation bit
        } else {
            accel_bias_reg[i] = accel_bias_reg[i] | 0x0001; // Preserve temperature compensation bit
        }
    }

    ESP_LOGI(TAG,"Calculated Offset %02x, %02x, %02x",accel_bias_reg[0],accel_bias_reg[1],accel_bias_reg[2]);

    // Push gyro biases to hardware registers
    uint8_t offset[6] = {   (uint8_t) (accel_bias_reg[0] >> 8 & 0xFF)   ,
                            (uint8_t )(accel_bias_reg[0] &  0xFF)       ,
                            (uint8_t )(accel_bias_reg[1] >> 8 &  0xFF)  ,
                            (uint8_t )(accel_bias_reg[1] &  0xFF)       ,
                            (uint8_t )(accel_bias_reg[2] >> 8 &  0xFF)  ,
                            (uint8_t )(accel_bias_reg[2] &  0xFF)       };

    ESP_LOGI(TAG,"New Offset is %02x%02x, %02x%02x, %02x%02x",offset[0],offset[1],offset[2],offset[3],offset[4],offset[5]);

    new_accel_bias_reg[0] = ((int16_t) offset[0] << 8) | offset[1];
    new_accel_bias_reg[1] = ((int16_t) offset[2] << 8) | offset[3];
    new_accel_bias_reg[2] = ((int16_t) offset[4] << 8) | offset[5];

    ESP_LOGI(TAG,"New Offset as float is %02f, %02f, %02f",(float)new_accel_bias_reg[0] * AMult,(float)new_accel_bias_reg[1]  * AMult,(float)new_accel_bias_reg[2] * AMult);

    if( mpu9250_write(XA_OFFSET_H,  data[0])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/ portTICK_PERIOD_MS);
    if( mpu9250_write(XA_OFFSET_L,  data[1])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_write(YA_OFFSET_H,  data[2])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_write(YA_OFFSET_L,  data[3])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_write(ZA_OFFSET_H,  data[4])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);
    if( mpu9250_write(ZA_OFFSET_L,  data[5])   != ESP_OK){ return ESP_FAIL;} vTaskDelay(1/portTICK_PERIOD_MS);

    */
    return ESP_OK;
}

//
//esp_err_t MPU9250_Calibrate(float * dest1,float * dest2)
//{
//    uint16_t ii, packet_count, fifo_count;
//    int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};
//
//    // reset device
//    // Write a one to bit 7 reset bit; toggle reset device
//    if( mpu9250_write(AK8963_CNTL1, 0x00)     != ESP_OK){ return ESP_FAIL;}
//    vTaskDelay(10/portTICK_PERIOD_MS)
//
//    // get stable time source; Auto select clock source to be PLL gyroscope reference if ready
//    // else use the internal oscillator, bits 2:0 = 001
//    writeRegister(PWR_MGMNT_1, CLOCK_SEL_PLL);
//    // enable accelerometer and gyro
//    writeRegister(PWR_MGMNT_2,SEN_ENABLE);
//    HAL_Delay(200);
//
//    // Configure device for bias calculation
//    writeRegister(MPUREG_INT_ENABLE, INT_DISABLE);   // Disable all interrupts
//    writeRegister(MPUREG_FIFO_EN, FIFO_DIS);      // Disable FIFO
//    writeRegister(MPUREG_PWR_MGMT_1, SEN_ENABLE);   // Turn on internal clock source
//    writeRegister(MPUREG_I2C_MST_CTRL, I2C_MST_DIS); // Disable I2C master
//    writeRegister(MPUREG_USER_CTRL, FIFO_DIS);    // Disable FIFO and I2C master modes
//    writeRegister(MPUREG_USER_CTRL, FIFO_RST);    // Reset FIFO and DMP
//    HAL_Delay(15);
//
//    // Configure MPU6050 gyro and accelerometer for bias calculation
//    writeRegister(MPUREG_CONFIG, 0x01);      // Set low-pass filter to 188 Hz
//    writeRegister(MPUREG_SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
//    writeRegister(MPUREG_GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
//    writeRegister(MPUREG_ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity
//
//    uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
//    uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g
//
//    // Configure FIFO to capture accelerometer and gyro data for bias calculation
//    writeRegister(MPUREG_USER_CTRL, 0x40);   // Enable FIFO
//    writeRegister(MPUREG_FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
//    HAL_Delay(40); // accumulate 40 samples in 40 milliseconds = 480 bytes
//
//    // At end of sample accumulation, turn off FIFO sensor read
//    writeRegister(MPUREG_FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
//    readRegisters(MPUREG_FIFO_COUNTH, 2,_buffer); // read FIFO sample count
//
//    fifo_count = ((uint16_t)_buffer[0] << 8) | _buffer[1];
//    packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging
//
//    for (ii = 0; ii < packet_count; ii++) {
//        int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
//        readRegisters(MPUREG_FIFO_R_W, 12,_buffer); // read data for averaging
//        accel_temp[0] = (int16_t) (((int16_t)_buffer[0] << 8) | _buffer[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
//        accel_temp[1] = (int16_t) (((int16_t)_buffer[2] << 8) | _buffer[3]  ) ;
//        accel_temp[2] = (int16_t) (((int16_t)_buffer[4] << 8) | _buffer[5]  ) ;
//        gyro_temp[0]  = (int16_t) (((int16_t)_buffer[6] << 8) | _buffer[7]  ) ;
//        gyro_temp[1]  = (int16_t) (((int16_t)_buffer[8] << 8) | _buffer[9]  ) ;
//        gyro_temp[2]  = (int16_t) (((int16_t)_buffer[10] << 8) | _buffer[11]) ;
//
//        accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
//        accel_bias[1] += (int32_t) accel_temp[1];
//        accel_bias[2] += (int32_t) accel_temp[2];
//        gyro_bias[0]  += (int32_t) gyro_temp[0];
//        gyro_bias[1]  += (int32_t) gyro_temp[1];
//        gyro_bias[2]  += (int32_t) gyro_temp[2];
//
//    }
//    accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
//    accel_bias[1] /= (int32_t) packet_count;
//    accel_bias[2] /= (int32_t) packet_count;
//    gyro_bias[0]  /= (int32_t) packet_count;
//    gyro_bias[1]  /= (int32_t) packet_count;
//    gyro_bias[2]  /= (int32_t) packet_count;
//
//    if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) accelsensitivity;}  // Remove gravity from the z-axis accelerometer bias calculation
//    else {accel_bias[2] += (int32_t) accelsensitivity;}
//
//    // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
//    _buffer[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
//    _buffer[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
//    _buffer[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
//    _buffer[3] = (-gyro_bias[1]/4)       & 0xFF;
//    _buffer[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
//    _buffer[5] = (-gyro_bias[2]/4)       & 0xFF;
//
//    // Push gyro biases to hardware registers
//    writeRegister(MPUREG_XG_OFFS_USRH, _buffer[0]);
//    writeRegister(MPUREG_XG_OFFS_USRL, _buffer[1]);
//    writeRegister(MPUREG_YG_OFFS_USRH, _buffer[2]);
//    writeRegister(MPUREG_YG_OFFS_USRL, _buffer[3]);
//    writeRegister(MPUREG_ZG_OFFS_USRH, _buffer[4]);
//    writeRegister(MPUREG_ZG_OFFS_USRL, _buffer[5]);
//
//    // Output scaled gyro biases for display in the main program
//    dest1[0] = (float) gyro_bias[0]/(float) gyrosensitivity;
//    dest1[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
//    dest1[2] = (float) gyro_bias[2]/(float) gyrosensitivity;
//
//    // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
//    // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
//    // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
//    // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
//    // the accelerometer biases calculated above must be divided by 8.
//
//    int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
//    readRegisters(MPUREG_XA_OFFSET_H, 2,_buffer); // Read factory accelerometer trim values
//    accel_bias_reg[0] = (int32_t) (((int16_t)_buffer[0] << 8) | _buffer[1]);
//    readRegisters(MPUREG_YA_OFFSET_H, 2,_buffer);
//    accel_bias_reg[1] = (int32_t) (((int16_t)_buffer[0] << 8) | _buffer[1]);
//    readRegisters(MPUREG_ZA_OFFSET_H, 2,_buffer);
//    accel_bias_reg[2] = (int32_t) (((int16_t)_buffer[0] << 8) | _buffer[1]);
//
//    uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
//    uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis
//
//    for(ii = 0; ii < 3; ii++) {
//        if((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
//    }
//
//    // Construct total accelerometer bias, including calculated average accelerometer bias from above
//    accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
//    accel_bias_reg[1] -= (accel_bias[1]/8);
//    accel_bias_reg[2] -= (accel_bias[2]/8);
//
//    _buffer[0] = (accel_bias_reg[0] >> 8) & 0xFF;
//    _buffer[1] = (accel_bias_reg[0])      & 0xFF;
//    _buffer[1] = _buffer[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
//    _buffer[2] = (accel_bias_reg[1] >> 8) & 0xFF;
//    _buffer[3] = (accel_bias_reg[1])      & 0xFF;
//    _buffer[3] = _buffer[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
//    _buffer[4] = (accel_bias_reg[2] >> 8) & 0xFF;
//    _buffer[5] = (accel_bias_reg[2])      & 0xFF;
//    _buffer[5] = _buffer[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers
//
//// Apparently this is not working for the acceleration biases in the MPU-9250
//// Are we handling the temperature correction bit properly?
//// Push accelerometer biases to hardware registers
//    writeRegister(MPUREG_XA_OFFSET_H, _buffer[0]);
//    writeRegister(MPUREG_XA_OFFSET_L, _buffer[1]);
//    writeRegister(MPUREG_YA_OFFSET_H, _buffer[2]);
//    writeRegister(MPUREG_YA_OFFSET_L, _buffer[3]);
//    writeRegister(MPUREG_ZA_OFFSET_H, _buffer[4]);
//    writeRegister(MPUREG_ZA_OFFSET_L, _buffer[5]);
//
//// Output scaled accelerometer biases for display in the main program
//    dest2[0] = (float)accel_bias[0]/(float)accelsensitivity;
//    dest2[1] = (float)accel_bias[1]/(float)accelsensitivity;
//    dest2[2] = (float)accel_bias[2]/(float)accelsensitivity;
//}


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