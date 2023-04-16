

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
#define MPU_PIN_NUM_CS   15
#define MPU_PIN_NUM_INT 26
#define MPU_SPI_DMA_CHAN    0

#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK  18
#define SD_PIN_NUM_CS   5

#define BOTON_1   27