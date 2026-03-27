/*
 * MODULE_LIST.c
 *
 *  Created on: 08-July-2025
 *      Author: Bavatharani
 */
#include "common.h"
#include "process_cmd.h"
#include "pin_config.h"
#include "em_eusart.h"
#include <math.h>
#include "em_burtc.h"
#include <ctype.h>
#include "App.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "em_iadc.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_FLASH_READ_SIZE 256
#define ADC_KSPS_MIN 1
#define ADC_KSPS_MAX 1000
#define CMD_PREFIX_LEN 13
#define CLK_SRC_ADC_FREQ          20000000
#define CLK_ADC_FREQ              10000000

#define IADC_INPUT_0_PORT_PIN     iadcPosInputPortBPin0;
#define IADC_INPUT_1_PORT_PIN     iadcNegInputPortBPin1;

#define IADC_INPUT_0_BUS          ABUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_ABUSALLOC_AEVEN0_ADC0
#define IADC_INPUT_1_BUS          ABUSALLOC
#define IADC_INPUT_1_BUSALLOC     GPIO_ABUSALLOC_AODD0_ADC0

system_state_t gSysState = {
    .ble_on = false,
    .gps_en = false,
    .gps_reset = false,
    .lcd_test = false,
    .hpwr = false,
    .mem_pwr = false,
    .keypad = false,
    .gps_on = false,
    .disp_vcc = false,
    .lcd_back_enabled = false,
    .pwr_mode = 0,
    .adc_type = 0,
    .adc_clk_rate = 500,
    .adc_sample_len = 0,
    .gAdcSamplesInt = 16,
    .gAdcSamplesExt = 16,
    .gAdcSamplesAc = 16,
    .gAdcClkRate   = 10000,
    .read_dc = false,
    .read_ac = false,
};
extern void flash_access_prepare(void);
volatile uint8_t gGPS_On = 0;
volatile uint16_t gpsRxIndex = 0;
volatile uint8_t gpsRxBuffer[GPS_RX_BUFFER_SIZE] = {0};
uint8_t flash_read_buf[MAX_FLASH_READ_SIZE];
bool lcd_initial = false;
static sl_sleeptimer_timer_handle_t em2_timer;
bool adc_init_flag = false;
/***************************************************************************//**
 * @brief
 *    Initialize the IADC peripheral (EFR32 internal ADC)
 *
 * @details
 *    This function performs the following steps:
 *    1. Enables the IADC0 clock in CMU.
 *    2. Resets IADC0 to a known default state.
 *    3. Configures the main IADC parameters (clock, reference voltage).
 *    4. Initializes the IADC0 peripheral with the configuration.
 *
 * @note
 *    The reference voltage is set to VDD (3.3V). ADC results will be in raw
 *    codes and must be converted to millivolts in the read function.
 ******************************************************************************/
void initIADC(void)
{
  // --- 1. Enable the IADC0 clock ---
  CMU_ClockEnable(cmuClock_IADC0, true);

  // --- 2. Reset IADC0 ---
  // Clears any previous configuration or state
  IADC_reset(IADC0);

  // --- 3. Create and configure the IADC initialization structure ---
  IADC_Init_t init = IADC_INIT_DEFAULT;              // Start with default values

  // Calculate and set source clock prescaler for ~10 MHz ADC clock
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, gSysState.gAdcClkRate, 0);

  // --- 4. Configure all channel configs ---
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;  // Default configs

  // Use VDD as reference (typical 3.3V)
  allConfigs.configs[0].reference = iadcCfgReferenceVddx;

  // Set actual reference voltage in millivolts (for internal conversions)
  allConfigs.configs[0].vRef = 3300;

  // --- 5. Initialize IADC0 with the configured structures ---
  // This programs the ADC and prepares it for single or differential reads
  IADC_init(IADC0, &init, &allConfigs);

  // Optional debug: could send UART message here to confirm initialization
  // send_uart_data((const uint8_t *)"IADC Initialized\r\n", 17);
}

/***************************************************************************//**
 * @brief
 *    Read a single IADC channel (single-ended or differential)
 *
 * @param[in] posInput
 *    Positive input selection
 * @param[in] negInput
 *    Negative input selection (ignored for single-ended)
 * @param[in] mode
 *    0 → single-ended (pos vs GND)
 *    1 → differential (pos vs neg)
 *
 * @return
 *    Converted voltage in mV
 *****************************************************************************/
int32_t readIADC_Channel(IADC_PosInput_t posInput,
                         IADC_NegInput_t negInput,
                         uint8_t mode)
{
  // Create default single conversion configuration
  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;

  // Only wait for 1 valid sample in FIFO
  initSingle.dataValidLevel = 1;

  // Trigger conversion once
  initSingle.triggerAction = iadcTriggerActionOnce;

  // Create single input configuration
  IADC_SingleInput_t input = IADC_SINGLEINPUT_DEFAULT;

  // Assign positive and negative input channels
  input.posInput = posInput;
  input.negInput = negInput;

  if(adc_init_flag == true)
    {
      // Allocate the analog bus for ADC0 inputs
      GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
      GPIO->IADC_INPUT_1_BUS |= IADC_INPUT_1_BUSALLOC;
    }

  // Initialize IADC single conversion
  IADC_initSingle(IADC0, &initSingle, &input);

  // Start single conversion
  IADC_command(IADC0, iadcCmdStartSingle);

  // Wait for conversion to complete and FIFO to have valid data
  while ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
      | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV);

  // Pull the result from the FIFO
  IADC_Result_t sample = IADC_pullSingleFifoResult(IADC0);

  // Get raw ADC value
  int32_t raw = sample.data;

  // Convert raw code to mV depending on mode
  if (mode == 1)            // differential
    return (raw * 3300) / (1 << 11);  // 12-bit differential full-scale
  else                      // single-ended
    return (raw * 3300) / 0xFFF;      // 12-bit single-ended full-scale
}
/**************************************************************************//**
 * @brief
 *    Print current system state over UART.
 *
 * @details
 *    This function checks the flags in the global `gSysState` structure
 *    and sends their status ("ON"/"OFF") over UART:
 *      - BLE advertising (ble_on)
 *      - GPS enable (gps_en)
 *
 * @return
 *    None
 *****************************************************************************/
void print_system_state(void)
{
  if (gSysState.ble_on)
    send_uart_data((const uint8_t *)"BLE=ON\r\n", sizeof("BLE=ON\r\n") - 1);
  else
    send_uart_data((const uint8_t *)"BLE=OFF\r\n", sizeof("BLE=OFF\r\n") - 1);

  if (gSysState.gps_en)
    send_uart_data((const uint8_t *)"GPS_EN=ON\r\n", sizeof("GPS_EN=ON\r\n") - 1);
  else
    send_uart_data((const uint8_t *)"GPS_EN=OFF\r\n", sizeof("GPS_EN=OFF\r\n") - 1);
}


// GPS_EN

/**************************************************************************//**
 * @brief
 *    GPS Enable Command
 *
 * @details
 *    - Sets system state gSysState.gps_en to true.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(GPS_EN, ON) to enable GPS_EN pin.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_en_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.gps_en = true; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: Gps_En_on() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(GPS_EN, ON); // Enable GPS_EN pin
}


/**************************************************************************//**
 * @brief
 *    GPS Disable Command
 *
 * @details
 *    - Sets system state gSysState.gps_en to false.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(GPS_EN, OFF) to disable GPS_EN pin.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_en_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.gps_en = false; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: Gps_En_off() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(GPS_EN, OFF); // Disable GPS_EN pin
}


// GPS_RESET

/**************************************************************************//**
 * @brief
 *    GPS Reset ON Command
 *
 * @details
 *    - Sets system state gSysState.gps_reset to true.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(GPS_RESET, ON) to assert GPS reset.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_reset_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.gps_reset = true; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: Gps_Reset_on() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(GPS_RESET, ON); // Assert GPS reset
}


/**************************************************************************//**
 * @brief
 *    GPS Reset OFF Command
 *
 * @details
 *    - Sets system state gSysState.gps_reset to false.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(GPS_RESET, OFF) to de-assert GPS reset.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_reset_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.gps_reset = false; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: Gps_Reset_off() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(GPS_RESET, OFF); // De-assert GPS reset
}


// MEM_PWR

/**************************************************************************//**
 * @brief
 *    Memory Power ON Command
 *
 * @details
 *    - Sets system state gSysState.mem_pwr to true.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(MEM_PWR, ON) to enable memory power.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void mem_pwr_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.mem_pwr = true; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: mem_pwr_on_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(MEM_PWR, ON); // Enable memory power
}


/**************************************************************************//**
 * @brief
 *    Memory Power OFF Command
 *
 * @details
 *    - Sets system state gSysState.mem_pwr to false.
 *    - Sends debug message over UART.
 *    - Calls Change_Power(MEM_PWR, OFF) to disable memory power.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void mem_pwr_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  gSysState.mem_pwr = false; // Update system state

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: mem_pwr_off_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF); // Brief delay

  Change_Power(MEM_PWR, OFF); // Disable memory power
}


// KEYPAD

/**************************************************************************//**
 * @brief
 *    Keypad ON Command
 *
 * @details
 *    - Sets system state gSysState.keypad to true.
 *    - Starts GPIO monitoring once to check key state.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void keypad_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused
  (void)len; // Unused

  gSysState.keypad = true;   // Enable keypad state
  gpio_monitor_once();        // Check key state once
}


/**************************************************************************//**
 * @brief
 *    Keypad OFF Command
 *
 * @details
 *    - Sets system state gSysState.keypad to false.
 *    - Sends acknowledgment over UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void keypad_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused
  (void)len; // Unused

  gSysState.keypad = false;  // Disable keypad state

  // Send acknowledgment
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nKEYPAD Disabled.\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
}

/**************************************************************************//**
 * @brief
 *    GPS ON Command
 *
 * @details
 *    - Enables GPS system state.
 *    - Resets GPS RX buffer index.
 *    - Enables EUSART2 RX interrupt for GPS reception.
 *    - Sends acknowledgment over UART.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused
  (void)len; // Unused

  gSysState.gps_on = true;  // Set system flag
  gpsRxIndex = 0;            // Reset RX buffer index

  EUSART2_enableGPSRx();     // Enable UART RX interrupt

  // Send acknowledgment
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nGPS=ON\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
}


/**************************************************************************//**
 * @brief
 *    GPS OFF Command
 *
 * @details
 *    - Clears GPS system state.
 *    - Disables EUSART2 RX interrupt.
 *    - Sends acknowledgment over UART.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void gps_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused
  (void)len; // Unused

  gSysState.gps_on = false;  // Clear system flag
  EUSART2_disableGPSRx();     // Disable UART RX interrupt

  // Send acknowledgment
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nGPS=OFF\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
}


/**************************************************************************//**
 * @brief
 *    Set ADC Type Command
 *
 * @details
 *    - Parses "ADC_TYPE=int" or "ADC_TYPE=ext" from command.
 *    - Updates gSysState.adc_type (0=internal, 1=external).
 *    - Sends ACK or ERR over UART.
 *
 * @param[in] cmd
 *    Pointer to command buffer containing ADC type.
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void adc_type_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len;

  char *param = (char *)cmd + 9;  // Skip "ADC_TYPE="

  // Skip whitespace after '='
  while (isspace((unsigned char)*param)) param++;

  if (strncmp(param, "ext", 3) == 0) {
      gSysState.adc_type = 1; // External ADC
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nACK:ADC_TYPE=ext (External)\r\n");
  }
  else if (strncmp(param, "int", 3) == 0) {
      gSysState.adc_type = 0; // Internal ADC
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nACK:ADC_TYPE=int (Internal)\r\n");
  }
  else {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nERR: Invalid ADC_TYPE. Use 'int' or 'ext'\r\n");
  }

  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}
/**************************************************************************//**
 * @brief
 *    SWOUT High Command
 *
 * @details
 *    - Sends debug message.
 *    - Calls Change_Power(SWOUT, ON) to enable the GPIO pin.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void ana_en_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: ana_en_on_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Enable SWOUT
  Change_Power(ANA_EN, ON);
}


/**************************************************************************//**
 * @brief
 *    SWOUT Low Command
 *
 * @details
 *    - Sends debug message.
 *    - Calls Change_Power(SWOUT, OFF) to disable the GPIO pin.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void ana_en_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: ana_en_off_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Disable SWOUT
  Change_Power(ANA_EN, OFF);
}




/**************************************************************************//**
 * @brief
 *    SWOUT High Command
 *
 * @details
 *    - Sends debug message.
 *    - Calls Change_Power(SWOUT, ON) to enable the GPIO pin.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void swout_high_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: swout_high_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Enable SWOUT
  Change_Power(SWOUT, ON);
}


/**************************************************************************//**
 * @brief
 *    SWOUT Low Command
 *
 * @details
 *    - Sends debug message.
 *    - Calls Change_Power(SWOUT, OFF) to disable the GPIO pin.
 *
 * @param[in] cmd
 *    Pointer to command buffer (unused).
 *
 * @param[in] len
 *    Length of command buffer (unused).
 *****************************************************************************/
void swout_low_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd;
  (void)len;

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: swout_low_cmd() called");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Disable SWOUT
  Change_Power(SWOUT, OFF);
}

/**************************************************************************//**
 * @brief
 *    DISP_VCC ON Command
 *
 * @details
 *    - Sets `gSysState.disp_vcc` to true.
 *    - Initializes the LCD and fills it with white color.
 *    - Prints debug message to UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void disp_vcc_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused parameter
  (void)len; // Unused parameter

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: disp_vcc_on_cmd() called\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Set internal state flag
  gSysState.disp_vcc = true;

  // Initialize LCD hardware
  LCD_Init();

  // Fill display with white color
  LCD_Draw_Single_Color(COLOR_WHITE);
}


/**************************************************************************//**
 * @brief
 *    DISP_VCC OFF Command
 *
 * @details
 *    - Sets `gSysState.disp_vcc` to false.
 *    - Powers down and disables LCD interface pins.
 *    - Fills the LCD with black.
 *    - Sends debug message over UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void disp_vcc_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused parameter
  (void)len; // Unused parameter

  rxDataReady = 0;                 // Clear any pending RX data
  if(GPIO_PinOutGet(LCD_VCC_EN_PORT, LCD_VCC_EN_PIN) == true)
    LCD_Draw_Single_Color(COLOR_BLACK); // Clear display

  // Disable LCD control lines
  GPIO_PinOutClear(LCD_VCC_EN_PORT, LCD_VCC_EN_PIN);
  GPIO_PinOutClear(LCD_DC_PORT, LCD_DC_PIN);
  GPIO_PinModeSet(LCD_SCK_PORT, LCD_SCK_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(LCD_MOSI_PORT, LCD_MOSI_PIN, gpioModeDisabled, 0);
  GPIO_PinModeSet(LCD_MISO_PORT, LCD_MISO_PIN, gpioModeDisabled, 0);

  // Debug message
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "DEBUG: disp_vcc_off_cmd() called\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  // Update system state
  gSysState.disp_vcc = false;

  // Turn off LCD power
  Change_Power(LCD_PWR, OFF);
}


/**************************************************************************//**
 * @brief
 *    Execute READ_DC command (DC voltage measurement)
 *
 * @details
 *    - Validates command format "READ_DCn" (n = 0–5).
 *    - Sends acknowledgment over UART.
 *    - Delegates measurement to internal or external ADC handler based
 *      on `gSysState.adc_type`.
 *
 * @param[in] cbuff
 *    Pointer to input command buffer.
 *
 * @param[in] len
 *    Length of the input command buffer.
 *****************************************************************************/
void read_dc_cmd(uint8_t *cbuff, uint16_t len)
{
  // Validate command prefix
  if (len < 8 || strncmp((char *)cbuff, "READ_DC", 7) != 0) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "ERR: Invalid command format\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  // Parse channel number (0–5)
  int channel = cbuff[7] - '0';
  if (channel < 0 || channel > 5) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "ERR: Invalid channel n=%d\r\n", channel);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  // Send acknowledgment
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "ACK:READ_DC%d Command Received\r\n", channel);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));

  // Call appropriate ADC handler
  if (gSysState.adc_type == 0) {
      read_dcn_cmd(cbuff, len); // Internal ADC
  } else {
      ADC_RDG_DC(cbuff, len);   // External ADC
  }
}

/**************************************************************************//**
 * @brief
 *    Execute READ_AC command (AC RMS voltage measurement)
 *
 * @details
 *    - Validates the "READ_ACn" command format.
 *    - Parses the channel number n (0–5).
 *    - Sends ACK to terminal.
 *    - Initiates AC measurement using internal or external ADC depending on
 *      `gSysState.adc_type`.
 *
 * @param[in] cbuff
 *    Pointer to input command buffer (expected: "READ_ACn").
 *
 * @param[in] len
 *    Length of the input command buffer.
 *****************************************************************************/
void read_ac_cmd(uint8_t *cbuff, uint16_t len)
{
  // Check command length and prefix
  if (len < 8 || strncmp((char *)cbuff, "READ_AC", 7) != 0) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "ERR: Invalid command format\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  // Parse channel number (0–5) from 8th character
  int channel = cbuff[7] - '0';
  if (channel < 0 || channel > 5) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "ERR: Invalid AC channel n=%d (Only 0 to 5 allowed)\r\n", channel);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  // Send acknowledgment over UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "ACK:READ_AC%d Command Received\r\n", channel);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));

  // Call appropriate AC measurement routine
  if (gSysState.adc_type == 0) {
      read_acn_cmd(channel);  // Internal ADC
  } else {
      ADC_RDG_AC(channel);    // External ADC
  }
}


/**************************************************************************//**
 * @brief
 *    Internal ADC DC voltage read command handler
 *
 * @details
 *    - Parses "READ_DCn" command (n = 0–5) to select ADC channel.
 *    - Performs single-ended and/or differential conversion.
 *    - Reports measured voltage (mV) and differential voltage (if applicable)
 *      over UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (expected: "READ_DCn").
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void read_dcn_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len;  // Unused parameter
  uint8_t n;  // ADC channel number (0–5)
  int32_t val = 0;      // Single-ended reading
  int32_t diff = 0;     // Differential reading
  gSysState.read_dc = true;  // Set DC read flag
  initIADC();
  // Parse channel number from command string like "READ_DC0"
  if (sscanf((char *)cmd, "READ_DC%hhu", &n) != 1 || n > 5) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nERROR: Invalid channel. Use 0–5\r\n");
      send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      gSysState.read_dc = false;
      return;
  }

  int32_t sum_val = 0;   // Accumulator for single-ended voltage
  int32_t sum_diff = 0;  // Accumulator for differential voltage
  uint8_t samples = gSysState.gAdcSamplesInt; // Number of samples to average

  switch (n) {
    case 0:
    case 1:
      sum_diff = 0;
      adc_init_flag = false;
      // Take multiple samples and accumulate
      for (uint8_t i = 0; i < samples; i++) {
          diff = readIADC_Channel(iadcPosInputPadAna0, (iadcNegInputPadAna1 | 1), 1); // Differential AIN0-AIN1
          sum_diff += diff;
          zdelay(50);
      }
      diff = sum_diff / samples;    // Compute average differential
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "Differential Value of Internal (AIN0-AIN1) AVG=%ld mV\r\n", diff);
      break;

    case 2:
    case 3:
      sum_diff = 0;
      adc_init_flag = false;
      // Take multiple samples and accumulate
      for (uint8_t i = 0; i < samples; i++) {
          diff = readIADC_Channel(iadcPosInputPadAna2, (iadcNegInputPadAna3 | 1), 1); // Differential AIN2-AIN3
          sum_diff += diff;
          zdelay(50);
      }
      diff = sum_diff / samples;
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "Differential Value of Internal (AIN2-AIN3) AVG=%ld mV\r\n", diff);
      break;

    case 4:
      sum_val = 0;
      adc_init_flag = true;
      // Take multiple samples and accumulate
      for (uint8_t i = 0; i < samples; i++) {
          val = readIADC_Channel(iadcPosInputPortAPin4, iadcNegInputGnd, 0); // Single-ended AIN4
          sum_val += val;
          zdelay(50);
      }
      val = sum_val / samples;
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "Internal BONDSHUNTp AVG=%ld mV\r\n", val);
      break;

    case 5:
      sum_val = 0;
      adc_init_flag = true;
      // Take multiple samples and accumulate
      for (uint8_t i = 0; i < samples; i++) {
          val = readIADC_Channel(iadcPosInputPortAPin6, iadcNegInputGnd, 0); // Single-ended AIN5
          sum_val += val;
          zdelay(50);
      }
      val = sum_val / samples;
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "Internal SHUNTp AVG=%ld mV\r\n", val);
      break;
  }

  // Send the averaged results over UART
  send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);  // Small delay to ensure UART transmission completes
  gSysState.read_dc = false; // Clear DC read flag
}


/**************************************************************************//**
 * @brief
 *    Perform AC RMS voltage measurement using IADC.
 *
 * @details
 *    - Reads N samples from a selected IADC channel based on the command index.
 *    - Calculates peak-to-peak (Vpp) and RMS (Vrms) voltages.
 *    - Uses global settings for number of samples (`gAdcSamplesAc`) and ADC clock rate (`gAdcClkRate`).
 *    - Sends results over UART.
 *
 * @param[in] cmd
 *    AC channel index (0–5) used to select IADC positive/negative inputs.
 *****************************************************************************/
void read_acn_cmd(uint8_t cmd)
{
  uint8_t n = cmd;                   // Channel index for AC measurement (0–5)
  gSysState.read_ac = true;          // Set AC read flag

  // Configuration parameters
  uint32_t numSamples = gSysState.gAdcSamplesAc;   // Number of AC samples to average
  uint32_t clkRate    = gSysState.gAdcClkRate;     // ADC sampling rate in Hz
  uint32_t sample_interval_us = 1000000UL / clkRate; // Delay between samples in microseconds

  int32_t  minVal =  0x7FFFFFFF;     // Initialize min value
  int32_t  maxVal = -0x7FFFFFFF;     // Initialize max value
  int64_t  sumSq  = 0;               // Sum of squares for RMS calculation
  int32_t  sampleVal;                // Single sample reading

  // Determine IADC channel based on command index
  IADC_PosInput_t posInput;
  IADC_NegInput_t negInput;
  uint8_t mode_select;
  initIADC();

  switch (n) {
    case 0:
    case 1:
      adc_init_flag = false;
      posInput    = iadcPosInputPadAna0;
      negInput    = iadcNegInputPadAna1 | 1; // Differential mode
      mode_select = 1;
      break;
    case 2:
    case 3:
      adc_init_flag = false;
      posInput    = iadcPosInputPadAna2;
      negInput    = iadcNegInputPadAna3 | 1; // Differential
      mode_select = 1;
      break;
    case 4:
      adc_init_flag = true;
      posInput    = iadcPosInputPortAPin4;
      negInput    = iadcNegInputGnd;         // Single-ended
      mode_select = 0;
      break;
    default:
      adc_init_flag = true;
      posInput    = iadcPosInputPortAPin6;
      negInput    = iadcNegInputGnd;         // Single-ended
      mode_select = 0;
      break;
  }

  // Sampling loop for averagin
  for (uint32_t i = 0; i < numSamples; i++) {
      // Read one sample from the selected IADC channel (in mV)
      sampleVal = readIADC_Channel(posInput, negInput, mode_select);

      // Track min and max for Vpp calculation
      if (sampleVal < minVal) minVal = sampleVal;
      if (sampleVal > maxVal) maxVal = sampleVal;

      // Accumulate sum of squares for RMS calculation
      sumSq += (int64_t)sampleVal * sampleVal;

      // Wait until next sample
      zdelay(sample_interval_us);
  }

  // Compute peak-to-peak voltage (Vpp) and RMS voltage (Vrms)
  int32_t vpp  = maxVal - minVal;                        // Vpp
  int32_t vrms = (int32_t)sqrt((double)sumSq / numSamples); // RMS

  // Format results into UART buffer
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "Internal READ_AC%u: Vpp=%ld mV, Vrms=%ld mV\r\n",n, (long)vpp, (long)vrms);

  // Send measurement results over UART
  send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));

  gSysState.read_ac = false;  // Clear AC read flag
}

/**************************************************************************//**
 * @brief
 *    Enable LCD Test Mode.
 *
 * @details
 *    - Sets `gSysState.lcd_test` to true.
 *    - Sends status message over UART.
 *    - Starts the LCD smiley/screensaver test pattern via `LCD_Testing_Bars()`.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void lcd_test_on_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused parameter
  (void)len; // Unused parameter

  // Enable LCD test mode
  gSysState.lcd_test = true;

  // Send status message over UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nLCD_TEST=ON\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));

  // Optional delay for UART stability
  zdelay(0xFFFF);

  // Start LCD test pattern / smiley screensaver
  LCD_Testing_Bars();
}

/**************************************************************************//**
 * @brief
 *    Disable LCD Test Mode.
 *
 * @details
 *    - Sets `gSysState.lcd_test` to false.
 *    - Sends status message over UART.
 *    - Fills the entire LCD display with white color.
 *
 * @param[in] cmd
 *    Pointer to input command buffer (unused).
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void lcd_test_off_cmd(uint8_t *cmd, uint16_t len)
{
  (void)cmd; // Unused parameter
  (void)len; // Unused parameter

  // Disable LCD test mode
  gSysState.lcd_test = false;

  // Send status message over UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nLCD_TEST=OFF\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));

  // Optional delay for UART transmission/stability
  zdelay(0xFFFF);

  // Fill entire LCD with white color to reset display
  LCD_Draw_Single_Color(COLOR_WHITE);
}

/**************************************************************************//**
 * @brief
 *    Set LCD backlight brightness.
 *
 * @details
 *    Parses the brightness percentage (0–100) from the command string:
 *       "LCD_BACK=nn"
 *
 *    - Clamps the value to valid range [0–100].
 *    - Updates the PWM duty cycle for the LCD backlight.
 *    - Updates system state variable `gSysState.lcd_back_enabled`.
 *    - Sends acknowledgment over UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer ("LCD_BACK=nn").
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void lcd_back_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len; // Prevent unused parameter warning

  // Parse brightness value as integer from command string
  // "LCD_BACK=" is 9 characters, so offset pointer by 9
  int value = atoi((char *)cmd + 9);

  // Clamp value to 0–100
  if (value < 0)   value = 0;
  if (value > 100) value = 100;

  // Convert to 8-bit unsigned for PWM duty cycle
  uint8_t brightness = (uint8_t)value;

  // Apply brightness to LCD backlight via PWM
  PWMsetDutyCycle(brightness);

  // Update system state flag
  gSysState.lcd_back_enabled = true;

  // Send acknowledgment over UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\nLCD_DISPLAY_BRIGHTNESS_PERCENT=%d\r\n", brightness);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));

  // Optional delay for stability (blocking)
  zdelay(0xFFFF);
}

/**************************************************************************//**
 * @brief
 *    Configure ADC sample length.
 *
 * @details
 *    Parses the number of ADC samples from the command string:
 *       "ADC_SAMPLE_LENGTH=nnnn"
 *
 *    - Validates that the number of samples is within [1–10000].
 *    - On success, updates the global system state variable
 *      `gSysState.gAdcSamplesAc`.
 *    - On error, sends descriptive error messages over UART.
 *
 * @param[in] cmd
 *    Pointer to input command buffer ("ADC_SAMPLE_LENGTH=nnnn").
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
void adc_sample_length_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len;                      // Suppress unused parameter warning
  uint16_t no_of_samples;          // Variable to hold parsed sample length

  // Parse "ADC_SAMPLE_LENGTH=<value>" into no_of_samples
  if (sscanf((char *)cmd, "ADC_SAMPLE_LENGTH=%hu", &no_of_samples) != 1)
    {
      // Parsing failed → print usage error
      int n = snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
                       "\r\nERR: Usage ADC_SAMPLE_LENGTH=<1–10000>\r\n");
      if (n > 0)
        send_uart_data((const uint8_t *)gcmdbuff, (size_t)n);
      return;                     // Exit early on parse error
    }

  // Validate that sample length is within 1–10000
  if (no_of_samples == 0 || no_of_samples > 10000)
    {
      // Invalid range → print error
      int n = snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
                       "\r\nERR: Invalid sample length (1–10000)\r\n");
      if (n > 0)
        send_uart_data((const uint8_t *)gcmdbuff, (size_t)n);
      return;                     // Exit early on invalid input
    }

  // Apply the validated sample length to system state
  gSysState.gAdcSamplesAc = no_of_samples;
  gSysState.gAdcSamplesInt = no_of_samples;
  gSysState.gAdcSamplesExt = no_of_samples;
  // Acknowledge update over UART
  int n = snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
                   "\r\nACK: ADC sample length set to %u\r\n", no_of_samples);
  if (n > 0)
    send_uart_data((const uint8_t *)gcmdbuff, (size_t)n);
}

/**************************************************************************//**
 * @brief
 *    Configure system power mode and optional delay.
 *
 * @details
 *    Parses a UART command of the form:
 *       "PWR_MODE=<mode> <delay>"
 *
 *    - Supported modes:
 *        - EM2 (2): Sleep mode with optional timed wake-up.
 *        - EM4 (4): Deep sleep mode with external wake-up/reset.
 *
 *    - The <delay> parameter (in seconds) is used only in EM2 mode.
 *      If omitted, defaults to 5 seconds.
 *
 *    Internally:
 *      - For EM2: Uses sleeptimer (RTC/CRYOTIMER) to wake up.
 *      - For EM4: Configures EM4, applies GPIO pulldowns, and enters EM4.
 *
 * @param[in] cbuff
 *    Pointer to command buffer containing "PWR_MODE=..." string.
 *
 * @param[in] len
 *    Length of the input command buffer.
 *****************************************************************************/

/* Forward declaration of EM2 wakeup callback */
static void em2_wakeup_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

/* ---------------- PWR_MODE Command ---------------- */
void pwr_mode_cmd(uint8_t *cbuff, uint16_t len)
{
  int mode = 0;                   // Stores parsed power mode (2 or 4)
  unsigned int delay = 0;         // Stores parsed delay in seconds

  char cmd_str[64] = {0};         // Local copy of command buffer
  size_t copy_len = (len > 63) ? 63 : len;  // Clamp copy size to avoid overflow
  memcpy(cmd_str, cbuff, copy_len);         // Copy input buffer locally
  cmd_str[copy_len] = '\0';       // Null terminate string

  // Strip CR/LF characters from end of command
  for (int i = 0; cmd_str[i]; i++)
    {
      if (cmd_str[i] == '\r' || cmd_str[i] == '\n')
        {
          cmd_str[i] = '\0';          // Replace CR/LF with null terminator
          break;                      // Stop scanning
        }
    }

  // Parse "PWR_MODE=<mode> <delay>"
  int parsed = sscanf(cmd_str, "PWR_MODE=%d %u", &mode, &delay);
  if (parsed < 1)  // Failed to parse at least <mode>
    {
      send_uart_data((const uint8_t*)"ERR: Bad PWR_MODE args\r\n",
                     sizeof("ERR: Bad PWR_MODE args\r\n") - 1);
      return;
    }

  if (mode == 2) // --------- EM2 Mode ---------
    {
      if (parsed == 1) delay = 5;   // Default delay = 5 seconds if not provided

      char buf[64];
      snprintf(buf, sizeof(buf), "Entering EM2 (delay=%u s)...\r\n", delay);
      send_uart_data((uint8_t*)buf, strlen(buf));

      // Convert delay (seconds) into ticks (32768 Hz clock)
      uint32_t ticks = delay * 32768;

      // Start sleeptimer for timed wake-up
      sl_status_t status = sl_sleeptimer_start_timer(
          &em2_timer,              // Timer handle
          ticks,                   // Timeout in ticks
          em2_wakeup_callback,     // Callback on wake-up
          NULL,                    // No user data
          0,                       // Flags
          0);                      // Priority
      if (status != SL_STATUS_OK)  // Failed to start timer
        {
          send_uart_data((const uint8_t*)"ERR: sleeptimer start fail\r\n",
                         sizeof("ERR: sleeptimer start fail\r\n") - 1);
          return;
        }

      // Enter EM2 sleep mode (will wake up on timer expiry or interrupt)
      sl_power_manager_sleep();
      //pwr.em2_wakeup_flag = true;
    }
  else if (mode == 4) // --------- EM4 Mode ---------
    {
      send_uart_data((const uint8_t*)"Entering EM4...\r\n",
                     sizeof("Entering EM4...\r\n") - 1);
      zdelay(0xFFFF);
      if (parsed == 1) delay = 3;
      pwr.Sleep_Counter = delay * 1000;

      // ← store user value immediately
      BURAM->RET[1].REG = pwr.Sleep_Counter;

      BURTC_CounterReset();
      BURTC_CompareSet(0, pwr.Sleep_Counter);

      __disable_irq();             // Disable interrupts before entering EM4

      EMU_EnterEM4();              // Enter EM4 (CPU halts until reset/wakeup)
    }
  else // --------- Invalid Mode ---------
    {
      send_uart_data((const uint8_t*)"ERR: Invalid mode\r\n",
                     sizeof("ERR: Invalid mode\r\n") - 1);
    }
}

/**************************************************************************//**
 * @brief
 *    Callback invoked when EM2 sleeptimer expires.
 *
 * @details
 *    This function runs after the configured delay in EM2 mode.
 *    It sets a global flag to signal that EM2 wakeup has occurred.
 *
 * @param[in] handle
 *    Timer handle (unused).
 *
 * @param[in] data
 *    User data pointer (unused).
 *****************************************************************************/
static void em2_wakeup_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;                  // Unused
  (void)data;                    // Unused
  pwr.em2_wakeup_flag = true;        // Signal EM2 wake-up
}

/**************************************************************************//**
 * @brief
 *    Write data to SPI flash memory
 *
 * @details
 *    Parses the command "FLASH_WRITE <data> <address>".
 *    Data is written in big-endian format, with automatic sector erase
 *    before writing. Supports writing 1–4 byte values.
 *
 * @param[in] cmd
 *    Pointer to command buffer ("FLASH_WRITE <data> <address>").
 *
 * @param[in] len
 *    Length of the input command buffer (unused).
 *****************************************************************************/
/* === Flash Write Command === */
void flash_write_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len;
  uint32_t data = 0, addr = 0;
  uint8_t data_buf[4];
  uint32_t byte_len = 0;
  // Ensure flash is powered
  init_spi_flash();
  flash_access_prepare();

  // Parse input: FLASH_WRITE=<data> <address>
  if (sscanf((char*)cmd, "FLASH_WRITE=%lx %lx", &data, &addr) == 2) {


      if (data <= 0xFF) {
          data_buf[0] = (uint8_t)data & 0xFF; byte_len = 1;
      } else if (data <= 0xFFFF) {
          data_buf[0] = (uint8_t)data & 0xFF;
          data_buf[1] = (uint8_t)(data >> 8) & 0xFF;
          byte_len = 2;
      } else if (data <= 0xFFFFFF) {
          data_buf[0] = (uint8_t)data & 0xFF;
          data_buf[1] = (uint8_t)(data >> 8) & 0xFF;
          data_buf[2] = (uint8_t)(data >> 16) & 0xFF;
          byte_len = 3;
      } else {
          data_buf[0] = (uint8_t)data & 0xFF;
          data_buf[1] = (uint8_t)(data >> 8) & 0xFF;
          data_buf[2] = (uint8_t)(data >> 16) & 0xFF;
          data_buf[3] = (uint8_t)(data >> 24) & 0xFF;
          byte_len = 4;
      }
      //Erase sector before programming
      flash_erase_sector(addr);
      flash_write_data(addr, data_buf, byte_len);

      // Verification
      uint8_t verify_buf[4];
      Flash_ReadData(addr, verify_buf, byte_len);

      char gcmdbuff[64];
      snprintf(gcmdbuff, sizeof(gcmdbuff),
               "\r\nFLASH_WRITE OK: 0x%06lX => 0x", addr);

      for (int i = byte_len - 1; i >= 0; i--) {
          char tmp[3];
          snprintf(tmp, sizeof(tmp), "%02X", verify_buf[i]);
          strcat(gcmdbuff, tmp);
      }
      strcat(gcmdbuff, "\r\n");
      send_uart_data((uint8_t*)gcmdbuff, strlen(gcmdbuff));
  } else {
      char gcmdbuff[64];
      snprintf(gcmdbuff, sizeof(gcmdbuff),
               "\r\nFLASH_WRITE ERROR: Usage => FLASH_WRITE=<data> <address>\r\n");
      send_uart_data((uint8_t*)gcmdbuff, strlen(gcmdbuff));
  }

  // Optional: delay after write
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief   Handle FLASH read command from UART
 *
 * This function parses a UART command of the form:
 *    FLASH_READ=<length> <address>
 * and reads data from external flash memory. The function ensures
 * that the flash is powered and initialized before access, and prints
 * the read data over UART in hexadecimal format.
 *
 * Example:
 *    FLASH_READ=16 0x1000
 *
 * @param[in]  cmd   Pointer to command buffer (ASCII string from UART)
 * @param[in]  len   Length of the command buffer
 *
 * @note Maximum read size is limited by MAX_FLASH_READ_SIZE.
 ******************************************************************************/
void flash_read_cmd(uint8_t *cmd, uint16_t len)
{
  (void)len;  // Suppress unused parameter warning

  uint32_t length = 0, addr = 0;  // Variables to store requested read length and flash address

  init_spi_flash();               // Initialize SPI interface for flash communication
  flash_access_prepare();         // Ensure external flash is powered and ready

  // Parse the input command: expect "FLASH_READ=<length> <address>"
  if (sscanf((char*)cmd, "FLASH_READ=%lx %lx", &length, &addr) == 2 &&
      length > 0 && length <= MAX_FLASH_READ_SIZE) {

      // Read 'length' bytes starting from 'addr' into flash_read_buf
      Flash_ReadData(addr, flash_read_buf, length);

      char gcmdbuff[128];  // Temporary buffer for UART output

      // Print the starting address for clarity
      snprintf(gcmdbuff, sizeof(gcmdbuff), "\r\nFLASH_READ 0x%06lX: ", addr);
      send_uart_data((uint8_t*)gcmdbuff, strlen(gcmdbuff));

      // Print each byte in hex format
      for (uint32_t i = 0; i < length; i++) {
          snprintf(gcmdbuff, sizeof(gcmdbuff), "%02X ", flash_read_buf[i]);
          send_uart_data((uint8_t*)gcmdbuff, strlen(gcmdbuff));
      }

      // End line after printing data
      send_uart_data((uint8_t*)"\r\n", 2);
  } else {
      // Command parsing failed or invalid length -> print usage error
      char gcmdbuff[128];
      snprintf(gcmdbuff, sizeof(gcmdbuff),
               "\r\nFLASH_READ ERROR: Usage => FLASH_READ=<length> <address>\r\n");
      send_uart_data((uint8_t*)gcmdbuff, strlen(gcmdbuff));
  }
}

/***************************************************************************//**
 * @brief   Set the sampling clock rate for the internal IADC
 *
 * This function parses a UART command of the form:
 *    ADC_CLK_RATE=<1–1000>
 * and updates the internal ADC clock rate. The IADC is re-initialized with
 * the new clock rate. Acknowledgment or error messages are sent over UART.
 *
 * Example:
 *    ADC_CLK_RATE=250
 *
 * @param[in]  cmd   Pointer to command buffer (ASCII string from UART)
 * @param[in]  len   Length of the command buffer
 *
 * @note The value is interpreted as kHz and stored in gSysState.gAdcClkRate.
 ******************************************************************************/
void ADC_CLK_RATE(uint8_t *cmd, uint16_t len)
{
  (void)len;
  uint32_t ksps;
  if (sscanf((char *)cmd, "ADC_CLK_RATE=%lu", &ksps) != 1) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nERR: Usage ADC_CLK_RATE=<1–1000>\r\n");
      send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  if (ksps < 1 || ksps > 1000) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nERR: Clock rate out of range (1–1000 kHz)\r\n");
      send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  gSysState.gAdcClkRate = ksps * 1000;

  // Re-initialize IADC with new clock
  IADC_Init_t init = IADC_INIT_DEFAULT;
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, gSysState.gAdcClkRate, 0);

  // Declare allConfigs locally
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;
  allConfigs.configs[0].reference = iadcCfgReferenceVddx;
  allConfigs.configs[0].vRef      = 3300;

  IADC_init(IADC0, &init, &allConfigs);

  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\nACK: Internal ADC clock set to %lu Hz\r\n",
           gSysState.gAdcClkRate);
  send_uart_data((uint8_t *)gcmdbuff, strlen((const char *)gcmdbuff));
}
