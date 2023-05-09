//The ESP32 has ESP32-WROOM and ESP32-WROVER series modules. Please pay attention to the following configurations with GPIOs.
//
//The WROOM-32/32D/32U series have 26 pins available for customers. Please note:
//
//GPIO6 ~ GPIO11 are used by the internal flash and cannot be used elsewhere;
//
//GPIO34, 35, 36 and 39 are input-only pins and cannot be used for outputs;
//
//The ESP32 has a built-in GPIO Matrix, and some peripheral interfaces can be connected to any free pins. That is, for hardware designs, there is no need to strictly distribute some functions on certain pins.

//Strapping Pins
//ESP32 has 6 strapping pins:
//• MTDI/GPIO12: internal pull-down
//• GPIO0: internal pull-up
//• GPIO2: internal pull-down
//• GPIO4: internal pull-down
//• MTDO/GPIO15: internal pull-up
//• GPIO5: internal pull-up
//
//  Software can read the value of these 6 bits from the register ”GPIO_STRAPPING”.
//  During the chip power-on reset, the latches of the strapping pins sample the voltage level as strapping bits of ”0”
//  or ”1”, and hold these bits until the chip is powered down or shut down. The strapping bits configure the device
//  boot mode, the operating voltage of VDD_SDIO and other system initial settings.
//  Each strapping pin is connected with its internal pull-up/pull-down during the chip reset. Consequently, if a strapping
//  pin is unconnected or the connected external circuit is high-impendence, the internal weak pull-up/pull-down
//  will determine the default input level of the strapping pins.
//  To change the strapping bit values, users can apply the external pull-down/pull-up resistances, or apply the host
//  MCU’s GPIOs to control the voltage level of these pins when powering on ESP32.
//  After reset, the strapping pins work as the normal functions pins.
//  Refer to Table 2 for detailed boot modes configuration by strapping pins.
//

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
#define MPU_PIN_NUM_CS   25
#define MPU_PIN_NUM_INT 26

#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK  18
#define SD_PIN_NUM_CS   21

#define ADC_PIN 33

#define BOTON_1   27