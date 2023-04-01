
#ifndef MPU9250_H
#define MPU9250_H

//  n       Number Pin Name         Pin Description

//  9       AD0 / SDO               I2C Slave Address LSB (AD0); SPI serial data output (SDO)
//  11      FSYNC                   Frame synchronization digital input. Connect to GND if unused.
//  12      INT                     Interrupt digital output (totem pole or open-drain)
//  13      VDD                     Power supply voltage and Digital I/O supply voltage
//  18      GND                     Power supply ground
//  22      nCS                     Chip select (SPI mode only)
//  23      SCL / SCLK              I2C serial clock (SCL); SPI serial clock (SCLK)
//  24      SDA / SDI               I2C serial data (SDA); SPI serial data input (SDI)

#define MPU_PIN_NUM_MISO_ADO 12
#define MPU_PIN_NUM_MOSI_SDA 13
#define MPU_PIN_NUM_CLK_SCL  14
#define MPU_PIN_NUM_CS   27

#define MPU_SPI_DMA_CHAN    0

// Experimental
#define FIXUP_INS_OFFSET

#define ROTATION_YAW	180
//#define  ROTATION_YAW	90

// MPU9250
#define MPU9250_ID      0x71
#define MPU92XX_ID      0x73

// MPU9250 registers
#define XG_OFFSET_H     0x13
#define XG_OFFSET_L     0x14
#define YG_OFFSET_H     0x15
#define YG_OFFSET_L     0x16
#define ZG_OFFSET_H     0x17
#define ZG_OFFSET_L     0x18

#define SMPLRT_DIV      0x19
#define MPU_CONFIG      0x1A
#define GYRO_CONFIG     0x1B
#define ACCEL_CONFIG    0x1C
#define ACCEL_CONFIG2   0x1D

#define FIFO_EN         0x23

#define I2C_MST_CTRL    0x24
#define I2C_SLV0_ADDR   0x25
#define I2C_SLV0_REG    0x26
#define I2C_SLV0_CTRL   0x27

#define I2C_SLV4_CTRL   0x34

#define INT_PIN_CFG     0x37
#define INT_ENABLE      0x38
#define INT_STATUS      0x3A

#define ACCEL_XOUT_H    0x3B
#define ACCEL_XOUT_L    0x3C
#define ACCEL_YOUT_H    0x3D
#define ACCEL_YOUT_L    0x3E
#define ACCEL_ZOUT_H    0x3F
#define ACCEL_ZOUT_L    0x40
#define TEMP_OUT_H      0x41
#define TEMP_OUT_L      0x42
#define GYRO_XOUT_H     0x43
#define GYRO_XOUT_L     0x44
#define GYRO_YOUT_H     0x45
#define GYRO_YOUT_L     0x46
#define GYRO_ZOUT_H     0x47
#define GYRO_ZOUT_L     0x48

#define EXT_SENS_DATA_00 0x49
#define I2C_SLV0_DO     0x63
#define I2C_MST_DELAY_CTRL 0x67

#define USER_CTRL       0x6A
#define PWR_MGMT_1      0x6B
#define PWR_MGMT_2      0x6C

#define FIFO_COUNTH     0x72
#define FIFO_COUNTL     0x73
#define FIFO_R_W        0x74

#define WHO_IM_I        0x75

#define XA_OFFSET_H     0x77
#define XA_OFFSET_L     0x78
#define YA_OFFSET_H     0x7A
#define YA_OFFSET_L     0x7B
#define ZA_OFFSET_H     0x7D
#define ZA_OFFSET_L     0x7E

// AK8963
#define AK8963_I2C_ADDR 0x0c
#define AK8963_ID       0x48

/* AK8963 registers */
#define AK8963_WIA      0x00
#define AK8963_HXL      0x03
#define AK8963_CNTL1    0x0A
#define AK8963_CNTL2    0x0B
#define AK8963_ASAX     0x10

#define GRAVITY_MSS     9.80665f

#define FILTER_CONVERGE_COUNT 2000

// accelerometer scaling for 16g range
#define MPU9250_ACCEL_SCALE_1G    (GRAVITY_MSS / 2048.0f)

/*
 *  PS-MPU-9250A-00.pdf, page 8, lists LSB sensitivity of
 *  gyro as 16.4 LSB/DPS at scale factor of +/- 2000dps (FS_SEL==3)
 */
static const float GYRO_SCALE = 0.0174532f / 16.4f;

static const float TEMP_SCALE = 1.0f / 333.87f;
#define TEMP_OFFSET 21.0f

#define AK8963_MILLIGAUSS_SCALE 10.0f
static const float ADC_16BIT_RESOLUTION = 0.15f;

// MPU9250 IMU data are big endian
#define be16_val(v, idx) ((int16_t)(((uint16_t)v[2*idx] << 8) | v[2*idx+1]))
// AK8963 data are little endian
#define le16_val(v, idx) ((int16_t)(((uint16_t)v[2*idx+1] << 8) | v[2*idx]))

struct sample {
    uint8_t d[14];
};

struct ak_sample {
    uint8_t d[6];
    uint8_t st2;
};

struct ak_asa {
    uint8_t a[3];
};

esp_err_t MPU9250_init(void);
esp_err_t MPU9250_reset();
uint8_t mpu9250_read(uint8_t reg);
esp_err_t mpu9250_write(uint8_t reg, uint8_t val);
esp_err_t mpu9250_readn(uint8_t reg, uint8_t *buf, size_t len);
bool mpu9250_ready(void);
int mpu9250_fifo_count(void);
bool mpu9250_read_fifo(struct sample *rx);
bool check_fifo(int t);
void mpu9250_fifo_reset(void);
esp_err_t mpu9250_start(void);


void slv0_readn(uint8_t reg, uint8_t size);
void slv0_write1(uint8_t reg, uint8_t out);
uint8_t ak8963_read(uint8_t reg);
void ak8963_write(uint8_t reg, uint8_t val);
void ak8963_read_sample_start(void);
bool ak8963_read_sample(struct ak_sample *rx);
bool ak8963_read_asa(struct ak_asa *rx);
void ak8963_start(void);

#endif //MPU9250_H