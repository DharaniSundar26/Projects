/*
 * common.h
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "stdio.h"
#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_timer.h"
#include "em_prs.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "em_chip.h"
#include "em_letimer.h"
#include "em_rmu.h"
#include "em_iadc.h"
#include "em_rtcc.h"
#include <stdlib.h>
#include <string.h>
#include "process_cmd.h"
#include "pin_config.h"
#include "idn_cmd.h"
#include "em_timer.h"
#include "sl_udelay.h"
#include"app.h"

/*
 * KEYPAD GPIO Configuration
 */
#define MAX_ROWS    4
#define MAX_COLS    4

// Rows PB0–PB3
#define ROW_PORT_0   gpioPortB
#define ROW_PIN_0    0
#define ROW_PORT_1   gpioPortB
#define ROW_PIN_1    1
#define ROW_PORT_2   gpioPortB
#define ROW_PIN_2    2
#define ROW_PORT_3   gpioPortB
#define ROW_PIN_3    3

// Columns PB4–PB7
#define COL_PORT_0   gpioPortB
#define COL_PIN_0    4
#define COL_PORT_1   gpioPortB
#define COL_PIN_1    5
#define COL_PORT_2   gpioPortB
#define COL_PIN_2    6
#define COL_PORT_3   gpioPortB
#define COL_PIN_3    7

#define RX_BUFFER_SIZE 80

/*
 *
 * Debug UART Configuration
 *
 */
#define CONSOLE_UART_TX_PORT   gpioPortC
#define CONSOLE_UART_TX_PIN    15
#define CONSOLE_UART_RX_PORT   gpioPortC
#define CONSOLE_UART_RX_PIN    14

#define CONSOLE_UART           EUSART1
#define CONSOLE_UART_BAUDRATE  115200
#define CONSOLE_UART_CLOCK     cmuClock_EUSART1
#define CONSOLE_UART_RX_IRQ    EUSART1_RX_IRQn
#define CONSOLE_UART_TX_IRQ    EUSART1_TX_IRQn
#define CONSOLE_UART_INDEX     1

/*
 *
 * SPI Configuration
 *
 */
typedef struct __spiconf__
{
  uint32_t freq;      /* SPI Frequency        */
  uint8_t  misoport;  /* MISO GPIO Port bank  */
  uint8_t  misopin;   /* MISO GPIO Pin        */
  uint8_t  miso[5];   /* MISO GPIO Pin name   */
  uint8_t  misopinloc;/* MISO GPIO Pin location */
  uint8_t  mosiport;  /* MOSI GPIO Port bank  */
  uint8_t  mosipin;   /* MOSI GPIO Pin        */
  uint8_t  mosi[5];   /* MOSI GPIO Pin name   */
  uint8_t  mosipinloc;/* MOSI GPIO Pin location */
  uint8_t  clkport;   /* CLK  GPIO Port bank  */
  uint8_t  clkpin;    /* CLK  GPIO Pin        */
  uint8_t  clk[5];    /* CLK GPIO Pin name    */
  uint8_t  clkpinloc; /* CLK  GPIO Pin location */
  uint8_t  csport;    /* CS  GPIO Port bank   */
  uint8_t  cspin;     /* CS  GPIO Pin         */
  uint8_t  cs[5];     /* CLK GPIO Pin name    */
  uint8_t  cspinloc; /* CLK  GPIO Pin location   */
  uint8_t  adccsport;    /* CS  GPIO Port bank   */
  uint8_t  adccspin;     /* CS  GPIO Pin         */
  uint8_t  adccs[5];     /* CLK GPIO Pin name    */
  uint8_t  adccspinloc; /* CLK  GPIO Pin location   */
  uint8_t  isopened;  /* SPI Enable           */
  uint8_t  adsgpioisopened; /*ads enable*/
  uint8_t  misoto;    /* MISO Timeout         */
  uint8_t  sel_port;  /* Selected Port        */
  uint8_t  rx_count;  /* Number of received data in bytes */
}spiconf;

typedef struct __CSGPIOPINName__
{
  uint8_t    *pincmdname;
  uint8_t    pinctrlval;
}CSGPIOPINName;


typedef struct __POWER_MODE__
{
  uint32_t    Sleep_Counter;
  bool em2_wakeup_flag;
}power_mode;

// Color definitions (RGB565)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

#define PI 3.14159265

/*!
 * Pin function index
 */
#define SPI_CLK    0
#define SPI_CS     1
#define SPI_MISO   2
#define SPI_MOSI   3
#define PIN_UNAVAIABLE      0xFF

/* --- LCD BACKLIGHT PWM --- */
#define LCD_PWM_PORT   gpioPortD
#define LCD_PWM_PIN    10
#define LCD_PWM_TIMER  TIMER0
#define PWM_FREQ 50000

// LCD Pins and Ports
#define LCD_SCK_PORT        gpioPortB
#define LCD_SCK_PIN         15

#define LCD_MOSI_PORT       gpioPortB
#define LCD_MOSI_PIN        14

#define LCD_MISO_PORT       gpioPortB
#define LCD_MISO_PIN        13

#define LCD_RESET_PORT      gpioPortD
#define LCD_RESET_PIN       0

#define LCD_DC_PORT         gpioPortD
#define LCD_DC_PIN          8

#define LCD_CS_PORT         gpioPortB
#define LCD_CS_PIN          7

#define LCD_VCC_EN_PORT     gpioPortD
#define LCD_VCC_EN_PIN      9

/*
 *  LCD register addresses
 */
#define GRAM_WRITE      0x22
#define DISP_CTRL       0x07
#define OSC_START       0x00
#define SLEEP_MODE      0x10
#define ENTRY_MODE      0x11
#define H_PORCH         0x16
#define V_PORCH         0x17
#define LCD_DRIVER      0x02
#define PWR_CTRL_1      0x03
#define DRIVER_OUTPUT   0x01
#define GATE_SCAN_POS   0x0F
#define CYCLE_CTRL      0x0B
#define PWR_CTRL_2      0x0C
#define PWR_CTRL_3      0x0D
#define PWR_CTRL_4      0x0E
#define PWR_CTRL_5      0x1E
#define V_ADDR          0x44
#define H_STR_ADDR      0x45
#define H_END_ADDR      0x46
#define RAM_XADDR       0x4E
#define RAM_YADDR       0x4F

#define LCD_WIDTH       240
#define LCD_HEIGHT      320

/*!
 * GPIO Direction INPUT/OUTPUT/NA
 */

#define FALSE       0
#define TRUE        1

#define GPS_EN      11
#define GPS_RESET   7
#define LCD_PWR     8
#define HPWR        4
#define SUSP        3
#define SWOUT       9
#define MEM_PWR     10
#define ANA_EN      12
#define ON          1
#define OFF         0

/*
 * IOs FREE / Assigned
 */
#define BOARD_ID          0x1E9
#define FW_VERSION        "FW v1.00"

/*
 *  SPI Flash Memory Macro Defines
 */

#define FLASH_CS_PORT       gpioPortA
#define FLASH_CS_PIN        10
#define FLASH_MISO_PORT     gpioPortA
#define FLASH_MISO_PIN      13
#define FLASH_MOSI_PORT     gpioPortA
#define FLASH_MOSI_PIN      14
#define FLASH_SCK_PORT      gpioPortA
#define FLASH_SCK_PIN       15
#define FLASH_TEST_ADDR     0x000000

#define CMD_READ_DATA        0x03
#define CMD_PAGE_PROGRAM     0x02
#define CMD_WRITE_ENABLE     0x06
#define CMD_READ_STATUS1     0x05
#define CMD_SECTOR_ERASE     0x20


/* GPIO Pin Macro */
#define GPIO_GPS_RESET_PORT    gpioPortC
#define GPIO_GPS_RESET_PIN     12
#define GPIO_DISP_VCC_PORT     gpioPortD
#define GPIO_DISP_VCC_PIN      9
#define GPIO_SWOUT_PORT        gpioPortD
#define GPIO_SWOUT_PIN         12
#define GPIO_MEM_PWR_PORT      gpioPortD
#define GPIO_MEM_PWR_PIN       11
#define GPIO_GPS_EN_PORT       gpioPortC
#define GPIO_GPS_EN_PIN        13

/*
 * External ADS124S06 - SPI
 */
#define ADS124S06_SPI_RX_PORT    gpioPortD
#define ADS124S06_SPI_RX_PIN     13
#define ADS124S06_SPI_TX_PORT    gpioPortD
#define ADS124S06_SPI_TX_PIN     14
#define ADS124S06_SPI_SCK_PORT   gpioPortD
#define ADS124S06_SPI_SCK_PIN    15
#define ADS124S06_DRDY_PORT      gpioPortB
#define ADS124S06_DRDY_PIN       9
#define ANALOG_ENABLE_PORT    gpioPortB
#define ANALOG_ENABLE_PIN     10

/*
 * GPS PIN configuration
 */
#define GPS_RX_BUFFER_SIZE        256
#define EUSART2_RX_PORT           gpioPortC
#define EUSART2_RX_PIN            11
#define EUSART2_TX_PORT           gpioPortC
#define EUSART2_TX_PIN            10

typedef struct
{
  bool ble_on;
  bool gps_en;
  bool gps_reset;
  bool lcd_test;
  bool hpwr;
  bool mem_pwr;
  bool keypad;
  bool gps_on;
  bool k_n[10];
  uint8_t pwr_mode;
  uint16_t adc_type;
  uint16_t adc_clk_rate;
  uint16_t adc_sample_len;
  uint16_t gAdcSamplesInt;
  uint16_t gAdcSamplesExt;
  uint16_t gAdcSamplesAc ;
  uint32_t gAdcClkRate   ;
  bool disp_vcc;
  bool lcd_back_enabled;
  bool read_dc;
  bool read_ac;
} system_state_t;

/* Command functions */
extern uint8_t rxBuffer[RX_BUFFER_SIZE];
extern bool rxDataReady;
extern volatile uint8_t  gGPS_On;
extern volatile uint8_t  gpsRxBuffer[GPS_RX_BUFFER_SIZE];
extern volatile uint16_t gpsRxIndex;
void delay_ms(uint32_t ms);
extern power_mode pwr;
extern system_state_t gSysState;
extern uint8_t advertising_set_handle;
void Change_Power(uint8_t device, uint8_t state);
void process_cmd(uint8_t *cbuff, uint16_t len);
/*
 *RELAY
 */
extern void sr_ch1_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_ch2_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_ch3_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_ch4_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_ch4S_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng1_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng2_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng4_relay_on(uint8_t *cbuff, uint16_t len);
extern void sr_kcal_relay_on(uint8_t *cbuff, uint16_t len);

extern void sr_ch1_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_ch2_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_ch3_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_ch4_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_ch4S_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng1_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng2_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_kcrng4_relay_off(uint8_t *cbuff, uint16_t len);
extern void sr_kcal_relay_off(uint8_t *cbuff, uint16_t len);
void All_relay_off();

/*
 *FLASH
 */
int8_t checkfornumber(char ch);
extern void flash_access_prepare(void);
void init_spi_flash(void);
void Flash_ReadJedecID(void);
void Flash_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);
void flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len);
void flash_power_on(void);
void flash_erase_sector(uint32_t addr);
extern void send_uart_data(const uint8_t *buff, size_t len);
extern void zdelay(uint32_t dly);
extern uint8_t gcmdbuff[100];

/*
 *KEYPAD
 */
void keypad_init(void);
char keypad_scan(void);
void process_keypad_loop(void);
void keypad_enable(bool en);
bool keypad_is_enabled(void);
void gpio_monitor_once(void);

/*
 * LCD
 */
void LCD_Testing_Bars(void);
void LCD_Init(void);
void LCD_Draw_Single_Color(uint16_t color);
void LCD_Draw_ColorBars_Horizontal_Cursor(void);
void PWMsetDutyCycle(uint8_t chan0);
void lcd_back_cmd(uint8_t *cmd, uint16_t len);
/*
 * ADC
 */
void ADC_RDG_DC(uint8_t *cbuff, uint16_t len);
void ADC_RDG_AC(uint8_t channel);
void zdelay(uint32_t dly);
void initIADC(void);
int32_t readIADC_Channel(IADC_PosInput_t posInput,IADC_NegInput_t negInput,uint8_t mode);
void read_dcn_cmd(uint8_t *cmd, uint16_t len);
void read_acn_cmd(uint8_t cmd);
extern void ads124_inp_adc(uint8_t *cbuff, uint16_t len);
extern void ads124_conf_adc(uint8_t *cbuff, uint16_t len);
void adc_type_cmd(uint8_t *cmd, uint16_t len);
void read_dc_cmd(uint8_t *cbuff, uint16_t len);
void ADC_READID(void);
/*
 * EM4
 */
void em4_wakeup_config(void);
/*
 * GPS
 */
void EUSART2_enableGPSRx(void);
void EUSART2_disableGPSRx(void);


#endif /* COMMON_H_ */
