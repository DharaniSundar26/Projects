/*
 * ads124_adc_cmd.h
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */
#ifndef ADS124_ADC_CMD_H_
#define ADS124_ADC_CMD_H_

#define NUM_OF_ADC_CMDS     12
#define PIN_UNAVAIABLE      0xFF

/*!
 * I2C TX/RX Buffer size
 */
#define ADS_SPI_BUFFER_SIZE     50


//READ_AC

#define ADC_REF_VOLT      2.5f
#define ADC_GAIN          1
#define AC_NUM_SAMPLES    10
#define AC_SAMPLE_DELAY   10   // optional delay between samples in ms
#define ADC_CLK_FREQ_HZ   1000000  // example value for SPI clock

#define PORT_NOT_SELECTED 0xFF
#define PIN_UNAVAIABLE    0xFF

spiconf spicnf =
    {
        1000000,
        0xFE,
        0xFE,
        " ",
        0xFE,
        0xFE,
        0xFE,
        " ",
        0xFE,
        0xFE,
        0xFE,
        " ",
        0xFE,
        0xFE,
        0xFE,
        " ",
        0xFE,
        0xFE,
        0xFE,
        " ",
        0xFE,
        FALSE,
        FALSE,
        10,
        PORT_NOT_SELECTED,
        0

    };

/*!
 * Pin function index
 */
#define SPI_CLK    0
#define SPI_CS     1
#define SPI_MISO   2
#define SPI_MOSI   3

#define FREQ_STR_SIZE 15

#define length   3

typedef enum {
  COMMAND
} readMode;

typedef struct __ADSCmds__
{
  uint8_t    *adscmdstr;
  void       (*ADSfunc)(uint8_t *, uint8_t);
}ADSCmds;

// ADS SPI configuration
typedef struct __ads_spi_pin_conf
{
  uint8_t  sel_port;
  uint32_t freq;
  uint8_t  *miso;
  uint8_t  *mosi;
  uint8_t  *clk;
  uint8_t  misoto;
  uint8_t  *srcs;
  uint8_t  *drdy;
  uint8_t  *analog_enable;

}ads_spi_pin_conf;

ads_spi_pin_conf ads_spiconf =
    {
        2,
        1000000,
        (uint8_t  *)"PD13",    //MISO
        (uint8_t  *)"PD14",    //MOSI
        (uint8_t  *)"PD15",    //CLK
        10,
        (uint8_t  *)"PB10",   // Relay chip select
        (uint8_t  *)"PB09",    //DRDY
        (uint8_t  *)"PB10"     //Analog Enable
    };


// GIPO DRDY configuration
typedef struct __drdy_conf
{
  uint8_t  drdyport;    /* DRDY  GPIO Port bank   */
  uint8_t  drdypin;     /* DRDY  GPIO Pin         */
  uint8_t  drdy[5];     /* DRDY GPIO Pin name    */
}drdy_conf;

drdy_conf Gpio_drdy_conf =
    {
        1,
        9,
        " "
    };

// GIPO Ana_ena configuration
typedef struct __Ana_ena
{
  uint8_t  ana_ena_port;    /* Analog Enable  GPIO Port bank   */
  uint8_t  ana_ena_pin;     /* Analog Enable  GPIO Pin         */
  uint8_t  ana_ena[5];      /* Analog Enable GPIO Pin name    */
}Ana_ena;

Ana_ena Gpio_ana_ena =
    {
        1,
        10,
        " "
    };

// GIPO Ana_ena configuration
typedef struct __reset
{
  uint8_t  reset_port;    /* Analog Enable  GPIO Port bank   */
  uint8_t  reset_pin;     /* Analog Enable  GPIO Pin         */
  uint8_t  reset[5];      /* Analog Enable GPIO Pin name    */
}reset;

reset Gpio_adc_reset =
    {
        3,
        2,
        " "
    };

// GIPO Relay CS configuration
typedef struct __srcs_conf
{
  uint8_t  srcsport;    /* DRDY  GPIO Port bank   */
  uint8_t  srcspin;     /* DRDY  GPIO Pin         */
  uint8_t  srcs[5];     /* DRDY GPIO Pin name    */
}srcs_conf;

srcs_conf Gpio_srcs_conf =
    {
        0xFE,
        45,
        " "
    };


/********************************************************************************//**
 *
 * @name Command byte definition
 * -----------------------------------  ----------------------------------------------
 * |  Bit 7  |  Bit 6  |  Bit 5  |  Bit 4  |  Bit 3  |  Bit 2  |  Bit 1  |  Bit 0  |
 * ---------------------------------------------------------------------------------
 * |                                  COMMAND                                      |
 * ---------------------------------------------------------------------------------
 */

// SPI Control Commands
#define OPCODE_NOP          ((uint8_t) 0x00)
#define OPCODE_WAKEUP       ((uint8_t) 0x02)
#define OPCODE_POWERDOWN    ((uint8_t) 0x04)
#define OPCODE_RESET        ((uint8_t) 0x06)
#define OPCODE_START        ((uint8_t) 0x08)
#define OPCODE_STOP         ((uint8_t) 0x0A)

/* ADS124S08 Register 0x1 (STATUS) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0   |
 *------------------------------------------------------------------------------------------------
 *|  FL_POR  |    nRDY   | FL_P_RAILP| FL_P_RAILN| FL_N_RAILP| FL_N_RAILN| FL_REF_L1 | FL_REF_L0 |
 *------------------------------------------------------------------------------------------------
 */
/** STATUS register address */
#define REG_ADDR_STATUS         ((uint8_t) 0x01)

/** STATUS default (reset) value */
#define STATUS_DEFAULT          ((uint8_t) 0x80)

/********************************************************************************//**
 *
 * @name Command byte definition
 * ---------------------------------------------------------------------------------
 * |  Bit 7  |  Bit 6  |  Bit 5  |  Bit 4  |  Bit 3  |  Bit 2  |  Bit 1  |  Bit 0  |
 * ---------------------------------------------------------------------------------
 * |                                  COMMAND                                      |
 * ---------------------------------------------------------------------------------
 */

// SPI Control Commands
#define OPCODE_NOP          ((uint8_t) 0x00)
#define OPCODE_WAKEUP       ((uint8_t) 0x02)
#define OPCODE_POWERDOWN    ((uint8_t) 0x04)
#define OPCODE_RESET        ((uint8_t) 0x06)
#define OPCODE_START        ((uint8_t) 0x08)
#define OPCODE_STOP         ((uint8_t) 0x0A)

//SPI Calibration Commands
#define OPCODE_SYOCAL       ((uint8_t) 0x16)
#define OPCODE_SYGCAL       ((uint8_t) 0x17)
#define OPCODE_SFOCAL       ((uint8_t) 0x19)

// SPI Data Read Command
#define OPCODE_RDATA        ((uint8_t) 0x12)

// SPI Register Read and Write Commands
#define OPCODE_RREG         ((uint8_t) 0x20)
#define OPCODE_WREG         ((uint8_t) 0x40)
#define OPCODE_RWREG_MASK   ((uint8_t) 0x1F)


#define ADS_FL_POR_MASK       0x80
#define ADS_nRDY_MASK         0x40
#define ADS_FL_P_RAILP_MASK   0x20
#define ADS_FL_P_RAILN_MASK   0x10
#define ADS_FL_N_RAILP_MASK   0x08
#define ADS_FL_N_RAILN_MASK   0x04
#define ADS_FL_REF_L1_MASK    0x02
#define ADS_FL_REF_L0_MASK    0x10



/* ADS124S08 Register 0x4 (DATARATE) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|   G_CHOP  |    CLK    |    MODE   |   FILTER  |           DR[3:0]                   |
 *-----------------------------------------------------------------------------------------------
 */
// DATARATE register address
#define REG_ADDR_DATARATE   ((uint8_t) 0x04)

/** DATARATE default (reset) value */
#define DATARATE_DEFAULT      ((uint8_t) 0x14)

#define ADS_GLOBALCHOP      0x80
#define ADS_CLKSEL_EXT      0x40
#define ADS_CONVMODE_SS     0x20
#define ADS_CONVMODE_CONT   0x00
#define ADS_FILTERTYPE_LL   0x10

/* ADS124S08 Register 0x2 (INPMUX) Definition
 *|   Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *------------------------------------------------------------------------------------------------
 *|             MUXP[3:0]            |            MUXN[3:0]                    |
 *------------------------------------------------------------------------------------------------
 */
// INPMUX register address
#define REG_ADDR_INPMUX     ((uint8_t) 0x02)

/** INPMUX default (reset) value */
#define INPMUX_DEFAULT          ((uint8_t) 0x01)

// Define the ADC positive input channels (MUXP)
#define ADS_P_AIN0        0x00
#define ADS_P_AIN1        0x10
#define ADS_P_AIN2        0x20
#define ADS_P_AIN3        0x30
#define ADS_P_AIN4        0x40
#define ADS_P_AIN5        0x50
#define ADS_P_AIN6        0x60
#define ADS_P_AIN7        0x70
#define ADS_P_AIN8        0x80
#define ADS_P_AIN9        0x90
#define ADS_P_AIN10       0xA0
#define ADS_P_AIN11       0xB0
#define ADS_P_AINCOM      0xC0

// Define the ADC negative input channels (MUXN)
#define ADS_N_AIN0        0x00
#define ADS_N_AIN1        0x01
#define ADS_N_AIN2        0x02
#define ADS_N_AIN3        0x03
#define ADS_N_AIN4        0x04
#define ADS_N_AIN5        0x05
#define ADS_N_AIN6        0x06
#define ADS_N_AIN7        0x07
#define ADS_N_AIN8        0x08
#define ADS_N_AIN9        0x09
#define ADS_N_AIN10       0x0A
#define ADS_N_AIN11       0x0B
#define ADS_N_AINCOM      0x0C

/* ADS124S08 Register 0xA (OFCAL0) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        OFC[7:0]                                             |
 *-----------------------------------------------------------------------------------------------
 */
// OFCAL0 register address
#define REG_ADDR_OFCAL0     ((uint8_t) 0x0A)

/** OFCAL0 default (reset) value */
#define OFCAL0_DEFAULT        ((uint8_t) 0x00)



/* ADS124S08 Register 0xB (OFCAL1) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        OFC[15:8]                                            |
 *-----------------------------------------------------------------------------------------------
 */
// OFCAL1 register address
#define REG_ADDR_OFCAL1     ((uint8_t) 0x0B)

/** OFCAL1 default (reset) value */
#define OFCAL1_DEFAULT        ((uint8_t) 0x00)


/* ADS124S08 Register 0xC (OFCAL2) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        OFC[23:16]                                           |
 *-----------------------------------------------------------------------------------------------
 */
// OFCAL2 register address
#define REG_ADDR_OFCAL2     ((uint8_t) 0x0C)

/** OFCAL2 default (reset) value */
#define OFCAL2_DEFAULT        ((uint8_t) 0x00)


/* ADS124S08 Register 0xD (FSCAL0) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        FSC[7:0]                                             |
 *-----------------------------------------------------------------------------------------------
 */
// FSCAL0 register address
#define REG_ADDR_FSCAL0     ((uint8_t) 0x0D)

/** FSCAL0 default (reset) value */
#define FSCAL0_DEFAULT        ((uint8_t) 0x00)


/* ADS124S08 Register 0xE (FSCAL1) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        FSC[15:8]                                            |
 *-----------------------------------------------------------------------------------------------
 */
// FSCAL1 register address
#define REG_ADDR_FSCAL1     ((uint8_t) 0x0E)

/** FSCAL1 default (reset) value */
#define FSCAL1_DEFAULT        ((uint8_t) 0x00)


/* ADS124S08 Register 0xF (FSCAL2) Definition
 *|  Bit 7   |   Bit 6   |   Bit 5   |   Bit 4   |   Bit 3   |   Bit 2   |   Bit 1   |   Bit 0  |
 *-----------------------------------------------------------------------------------------------
 *|                                        FSC[23:16]                                           |
 *-----------------------------------------------------------------------------------------------
 */
// FSCAL2 register address
#define REG_ADDR_FSCAL2     ((uint8_t) 0x0F)

/** FSCAL2 default (reset) value */
#define FSCAL2_DEFAULT        ((uint8_t) 0x40)


#define COMMAND_LENGTH   2
#define MAX_REG_ADDRESS  0x0F
#define NUM_SAMPLE       20

#define ADC_CHANNEL_P         ADS_P_AIN0
#define ADC_CHANNEL_N         ADS_P_AIN1
#define ADC_CHANNEL_SHUNT     ADS_P_AIN2
#define ADC_CHANNEL_REF       ADS_P_AIN3


/*
 * Data to be read for the each channel
 */
#define ADC_24BIT_DATA_LEN    3
#define ADC_two_24BIT_DATA_LEN    6
/*
 * Bit resolution 2^(24-1)
 * Reference Voltage 3.3 (REFP0)
 */
#define BIT_RES   8388608
#define REF_VOLT  2.5
#define REF_VOLT1 3.3

/*
 * Number of samples to be taken for the calibration
 */
#define    NUM_CAL_SAMPLE 50


// Declaration of ADS command functions
void ADC_CFG(uint8_t *cbuff, uint16_t len);
void ADC_CAL(uint8_t *cbuff, uint16_t len);
void ADC_RESET(uint8_t *cbuff, uint16_t len);
void ADC_CLK(uint8_t *cbuff, uint16_t len);
void ADC_STATUS(uint8_t *cbuff, uint16_t len);
void ADS124RWReg(uint8_t *cbuff, uint16_t len);
void ADC_WR(uint8_t reg, uint8_t val);
void ADC_RD(uint8_t reg);
void ADC_RD_ALL();
void ADC_SHUNT(uint8_t *cbuff, uint16_t len);
void ADC_REF(uint8_t *cbuff, uint16_t len);


int8_t sendCommand(uint8_t op_code);
int8_t ADS_SPI_Open();
int8_t init_ads_gpio();
bool waitForDRDYHtoL();
bool readConvertedData(uint8_t read_len);
bool read_config_reg(uint8_t reg);
bool write_config_reg(uint8_t reg, uint8_t val);
bool readChanneldata(uint8_t  channel_no, int32_t *dataValue1);
bool readPNChanneldata(uint8_t  channel_no, uint32_t *dataValuep,  uint32_t *dataValuen);

void read_dc_cmd(uint8_t *cbuff, uint16_t len);
void read_ac_cmd(uint8_t *cbuff, uint16_t len);
static uint8_t ADC_SPI_TxRx(uint8_t data);
/*!
 * ADS124 Command string
 *
 *   CONF:ADS124:CFG=<configuration>
 *   CONF:ADS124:CAL?=<calibration>
 *   CONF:ADS124:RESET=<reset>
 *   CONF:ADS124:CLK=<clock>
 *   CONF:ADS124:STATUS=<status>
 *   CONF:ADS124:WRREG=<wrregister>
 *   CONF:ADS124:RDREG=<rdregister>
 *   CONF:ADS124:RDALLREG=<rdallregister>
 *   INP:ADS124:DC?=<readadcDC>
 *   INP:ADS124:AC?=<readadcAC>
 *   INP:ADS124:SHUNT?=<readadcshuntinput>
 *   INP:ADS124:REF?=<readadcAREFinput>

 */

ADSCmds ads124adccmds[NUM_OF_ADC_CMDS]=
    {
        {(uint8_t *)"CONF:ADS124:CFG",      (void *)ADC_CFG},
        {(uint8_t *)"CONF:ADS124:CAL?",     (void *)ADC_CAL},
        {(uint8_t *)"CONF:ADS124:RESET",    (void *)ADC_RESET},
        {(uint8_t *)"CONF:ADS124:CLK ",     (void *)ADC_CLK},
        {(uint8_t *)"CONF:ADS124:STATUS",   (void *)ADC_STATUS},
        {(uint8_t *)"CONF:ADS124:REG",      (void *)ADS124RWReg},
        {(uint8_t *)"INP:ADS124:DC?",       (void *)ADC_RDG_DC},
        {(uint8_t *)"INP:ADS124:AC?",       (void *)ADC_RDG_AC},
        {(uint8_t *)"INP:ADS124:SHUNT?",    (void *)ADC_SHUNT},
        {(uint8_t *)"INP:ADS124:REF?",      (void *)ADC_REF}
    };





#endif /* ADS124_ADC_CMD_H_ */

