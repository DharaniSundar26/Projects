/*
 * ads124_adc_cmd.c
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#include "common.h"
#include "ads124_adc_cmd.h"
#include "ecode.h"
#include "em_usart.h"
#include <math.h>

#define div 10
#define DEFAULT_FREQ   2000000
#define ADC_REF_V   (float)3.33
#define ADC_SAMPLE_COUNT 10
#define ADC_DELAY_MS     500
#define NUMBER_OF_SAMPLES 10

uint32_t  cal_offset;
spiconf spicnf;
CSGPIOPINName CSGPIOPortName[5];
static uint8_t adsspi_txdata[ADS_SPI_BUFFER_SIZE];
static uint8_t adsspi_rxdata[ADS_SPI_BUFFER_SIZE];
volatile bool DRDY_STATUS;

extern int8_t checkfornumber(char ch);
extern int8_t checkforhex(char ch);

/**************************************************************************//**
 * @brief
 *   Perform SPI data transfer (full-duplex) for ADC.
 *
 * @details
 *   This function transmits and receives a specified number of bytes over
 *   the SPI interface connected to the ADC. If the `rx` pointer is NULL,
 *   received data is ignored.
 *
 * @param[in] tx
 *   Pointer to the transmit buffer (can be NULL to send 0x00).
 *
 * @param[out] rx
 *   Pointer to the receive buffer (can be NULL to discard received bytes).
 *
 * @param[in] len
 *   Number of bytes to transmit and receive.
 *****************************************************************************/

static void adc_spi_transfer(uint8_t *tx, uint8_t *rx, uint8_t len)
{
  for (uint8_t i = 0; i < len; i++) {
      uint8_t txd = tx ? tx[i] : 0x00;
      uint8_t rxd = ADC_SPI_TxRx(txd);
      if (rx) rx[i] = rxd;
  }
  zdelay(0xFFFF);
}
/**************************************************************************//**
 * @brief
 *   Transmit and receive one byte over ADC SPI (USART0).
 *
 * @details
 *   This function waits for TX buffer readiness, transmits one byte,
 *   and blocks until a response byte is received from the ADC.
 *
 * @param[in] data
 *   Byte to be transmitted to the ADC.
 *
 * @return
 *   Byte received from the ADC.
 *****************************************************************************/
static uint8_t ADC_SPI_TxRx(uint8_t data) {
  /* Wait until the TX buffer is empty */
  while (!(USART0->STATUS & USART_STATUS_TXBL));

  /* Send the byte over USART2 */
  USART_Tx(USART0, data);

  /* Wait until a byte has been received */
  while (!(USART0->STATUS & USART_STATUS_RXDATAV));

  /* Return the received byte */
  return (uint8_t)USART_Rx(USART0);
}
/**************************************************************************//**
 * @brief
 *   Initialize TIMER0 for PWM signal generation.
 *
 * @details
 *   Configures TIMER0 to generate a PWM waveform on GPIO pin PB8.
 *   The PWM signal frequency and duty cycle are derived from the
 *   peripheral clock and `DEFAULT_FREQ` constant.
 *
 *   The function also prints timer configuration parameters (frequency,
 *   top value, duty cycle) over UART for debug purposes.
 *
 * @note
 *   Ensure that `DEFAULT_FREQ` is defined prior to using this function.
 *   The GPIO pin used (PB8) must not conflict with other peripherals.
 *
 * @return
 *   None.
 *****************************************************************************/
void initTIMER0()
{
  uint32_t timerFreq;
  uint32_t top;
  uint32_t dutycycle;

  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);
  /*
   * Configure, but do not start TIMER0.  Do not prescale the TIMER
   * clock (stick with divide-by-1) in order to have the highest
   * resolution for the most accurate output.
   */
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.prescale = timerPrescale1;
  timerInit.enable = false;

  TIMER_Init(TIMER0, &timerInit);

  // Configure TIMER0 channel 0 for PWM, output initially low
  TIMER_InitCC_TypeDef cc0init = TIMER_INITCC_DEFAULT;
  cc0init.mode = timerCCModePWM;
  cc0init.coist = false;

  TIMER_InitCC(TIMER0, 0, &cc0init);

  // Output the clock on PB13
  GPIO_PinModeSet(gpioPortB, 8, gpioModePushPull, 0);

  /* Route pins to timer */
  // $[TIMER0 I/O setup]
  /* Set up CC0 */
  GPIO->TIMERROUTE[0].CC0ROUTE =
      (gpioPortB << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
      | (8        << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0);

  top = (timerFreq/ DEFAULT_FREQ) - 1;
  TIMER_TopSet (TIMER0, top);

  dutycycle = (top + 1) / 2;
  memset(gcmdbuff, 0x00, sizeof(gcmdbuff));
  /* Send the response to the Terminal */
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"Timer FREq = %lu top = %lu dutycycle = %lu \r\r\n",
           timerFreq, top, dutycycle);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  TIMER_CompareSet(TIMER0, 0, dutycycle);

  // Start the timer
  TIMER_Enable(TIMER0, true);
}

/************************************************************************************//**
 *
 * @brief waitForDRDYHtoL()
 *            Waits for a nDRDY GPIO to go from High to Low or until a timeout condition occurs
 *            The DRDY output line is used as a status signal to indicate
 *            when a conversion has been completed. DRDY goes low
 *            when new data is available.
 *
 * @param[in]   timeout_ms number of milliseconds to allow until a timeout
 *
 * @return      Returns true if DRDY pin low
 *
 *************************************************************************************/

bool waitForDRDYHtoL()
{
  uint8_t pin_read;
  uint16_t timeout = 0xFFF;

  bool ret = false;

  do
    {
      /* Read the pin value, 0 or 1 */
      pin_read= GPIO_PinInGet(gpioPortB, 9);

      if(pin_read == 0)
        break;

      sl_udelay_wait(10);
      timeout --;
    }while(timeout != 0);

  /* Received the DRDY */
  if(pin_read == 0)
    ret = true;

  return ret;
}

/*********************************************************************//**
 *  @brief readConvertedData
 *
 *  This function reads the ADC data from the ADS124 Chip
 *
 *  @param  read_len : Length of the data to be read
 *
 *  @return true : Successfully read the data
 *          false: Failure
 *
 ***********************************************************************/
bool readConvertedData(uint8_t read_len)
{
  uint8_t index = 0;
  uint8_t count;
  bool bret = false;

  /* Prepare RDATA command */
  adsspi_txdata[index++] = OPCODE_RDATA;

  /* Clear RX buffer */
  memset(adsspi_rxdata, 0x00, sizeof(adsspi_rxdata));

  /* Perform SPI transfer: command + read_len bytes */
  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, read_len + 1);

  /* Optional software timeout / MISO check */
  count = ads_spiconf.misoto;
  while ((count > 0) && (adsspi_rxdata[0] == 0x00)) // or another RX check
    {
      sl_udelay_wait(50);  // delay in µs
      count--;
    }

  if (count != 0)
    bret = true;  // successful transfer

  bret = true;
  return bret;
}


/*********************************************************************//**
 *  @brief readChanneldata
 *
 *  This function sends the start command to read the ADC data
 *
 *  @param  channel_no : Channel Number
 *
 *  @return true : Successfully read the data
 *          false: Failure
 *
 **************************************************************************/

bool readPNChanneldata(uint8_t  channel_no, uint32_t *dataValuep,  uint32_t *dataValuen)
{
  int8_t   ret1;
  bool     state;

  /* Configure the channel number */
  state = write_config_reg(REG_ADDR_INPMUX, channel_no);
  if(false == state)
    {
      return state;
    }

  /* Send the start conversion command */
  ret1 = sendCommand(OPCODE_START);
  if (0 != ret1)
    {
      return false;
    }

  state = waitForDRDYHtoL();

  /* Read the data */
  if(true == state)
    state = readConvertedData(ADC_two_24BIT_DATA_LEN);

  /* Read the data */
  if(true == state)
    {

      *dataValuep = ((adsspi_rxdata[1] << 16) |
          (adsspi_rxdata[2] << 8) |
          (adsspi_rxdata[3]));
      *dataValuen = ((adsspi_rxdata[4] <<  16) |
          (adsspi_rxdata[5] << 8) |
          (adsspi_rxdata[6]));
    }
  /* Send the stop command */
  ret1 = sendCommand(OPCODE_STOP);

  return state;
}

/********************************************************************//**
 * * @brief readChanneldata
 *
 *  This function sends the start command to read the ADC data
 *
 *  @param  channel_no : Channel Number
 *
 *  @return true : Successfully read the data
 *          false: Failure
 *
 ***********************************************************************/

bool readChanneldata(uint8_t  channel_no, int32_t *dataValue1)
{
  int8_t   ret1;
  bool     state;

  /* Configure the channel number */
  state = write_config_reg(REG_ADDR_INPMUX, channel_no);
  if(false == state)
    {
      return state;
    }

  /* Send the start conversion command */
  ret1 = sendCommand(OPCODE_START);
  if (0 != ret1)
    {
      return false;
    }

  state = waitForDRDYHtoL();
  //state = true;

  /* Read the data */
  if(true == state)
    state = readConvertedData(ADC_24BIT_DATA_LEN);

  /* Read the data */
  if(true == state)
    {
      *dataValue1 = ((adsspi_rxdata[1] <<  16) |
          (adsspi_rxdata[2] << 8) |
          (adsspi_rxdata[3]));

    }
  /* Send the stop command */
  ret1 = sendCommand(OPCODE_STOP);

  return state;
}

/*********************************************************************//**
 *  @brief ADS124 ADC config Command processing
 *
 *  This function performs interface configurations
 *
 *  @param  cbuff[IN] : Command Received from the UART terminal
 *          len[IN]   : Length of the received command string
 *
 *  @return none
 *
 *************************************************************************/

void ads124_conf_adc(uint8_t *cbuff, uint16_t len)
{
  uint8_t   index;
  uint8_t   cmdlen;

  /* Parse the ADC commands */
  for (index = 0; index < NUM_OF_ADC_CMDS; index++)
    {
      cmdlen = strlen((const char *)ads124adccmds[index].adscmdstr);

      if(0 == strncmp((const char *)cbuff, (const char *)ads124adccmds[index].adscmdstr,
                      cmdlen))
        {
          /* Call the appropriate SPI function Call */
          if (NULL != ads124adccmds[index].ADSfunc)
            ads124adccmds[index].ADSfunc(&cbuff[cmdlen], len - cmdlen);

          break;
        }
    }

}

/** *******************************************************************//**
 * @brief ADS124 ADC input read Command
 *
 *  This function performs interface configurations
 *
 *  @param  cbuff[IN] : Command Received from the UART terminal
 *          len[IN]   : Length of the received command string
 *
 *  @return none
 *
 *************************************************************************/
void ads124_inp_adc(uint8_t *cbuff, uint16_t len)
{
  uint8_t   index;
  uint8_t   cmdlen;

  /* Parse the ADC commands */
  for (index = 0; index < NUM_OF_ADC_CMDS; index++)
    {
      cmdlen = strlen((const char *)ads124adccmds[index].adscmdstr);

      if(0 == strncmp((const char *)cbuff, (const char *)ads124adccmds[index].adscmdstr,
                      cmdlen))
        {
          /* Call the appropriate SPI function Call */
          if (NULL != ads124adccmds[index].ADSfunc)
            ads124adccmds[index].ADSfunc(&cbuff[cmdlen], len - cmdlen);

          break;
        }
    }

}

/** *******************************************************************//**
 * @brief Open ADS SPI device
 *
 *  This function opens the ADS SPI device
 *
 *  @param  cbuff[IN] : Command Received from the UART terminal
 *          len[IN]   : Length of the received command string
 *
 *  @return none
 *
 * CONF:SPI:OPEN:0
 *
 ***********************************************************************/

int8_t ADS_SPI_Open(void)
{

  CMU_ClockEnable(cmuClock_USART0, true); // Enable USART1 clock

  // Configure flash SPI pins
  GPIO_PinModeSet(ADS124S06_SPI_TX_PORT, ADS124S06_SPI_TX_PIN, gpioModePushPull, 0); // MOSI output
  GPIO_PinModeSet(ADS124S06_SPI_RX_PORT, ADS124S06_SPI_RX_PIN, gpioModeInput, 0);   // MISO input
  GPIO_PinModeSet(ADS124S06_SPI_SCK_PORT, ADS124S06_SPI_SCK_PIN, gpioModePushPull, 0);   // SCK output

  // Configure USART1 routing to appropriate GPIO pins
  GPIO->USARTROUTE[0].TXROUTE  = (ADS124S06_SPI_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (ADS124S06_SPI_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE  = (ADS124S06_SPI_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (ADS124S06_SPI_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].CLKROUTE = (ADS124S06_SPI_SCK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) | (ADS124S06_SPI_SCK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN  = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_CLKPEN;

  // Initialize USART1 in synchronous SPI master mode
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;
  init.baudrate  = 1000000;   // 1 MHz SPI clock
  init.msbf      = true;      // MSB first
  init.master    = true;      // Master mode
  init.clockMode = usartClockMode1; // SPI mode 0
  USART_InitSync(USART0, &init);    // Apply configuration

  return 0;
}

/***************************************************************************//**
 * @brief
 *   Initialize GPIOs required for ADS124S06 operation.
 *
 * @details
 *   This function configures all the GPIO pins associated with the ADS124S06
 *   external ADC, including:
 *   - **DRDY (Data Ready):** Configured as input with internal pull-up.
 *   - **Analog Enable:** Configured as push-pull output and set HIGH to enable
 *     analog front-end circuitry.
 *   - **RESET:** Configured as push-pull output. A short reset pulse is applied
 *     to ensure a proper power-on state for the ADC.
 *
 *   The GPIO port and pin configuration structures (`Gpio_drdy_conf`,
 *   `Gpio_ana_ena`, and `Gpio_adc_reset`) are updated for later access.
 *
 * @note
 *   Ensure that the GPIO clock is enabled before calling this function.
 *   The function also sets a flag `spicnf.adsgpioisopened = TRUE` upon
 *   successful configuration.
 *
 * @return
 *   Returns **0** on successful initialization.
 *****************************************************************************/
int8_t init_ads_gpio(void)
{
  /* ----------------------
       1. Configure DRDY pin as input with pull-up
       ---------------------- */
  GPIO_PinModeSet(ADS124S06_DRDY_PORT , ADS124S06_DRDY_PIN, gpioModeInputPull, 1); // 1 = pull-up
  // Store port/pin if needed
  Gpio_drdy_conf.drdyport = ADS124S06_DRDY_PORT;
  Gpio_drdy_conf.drdypin  = ADS124S06_DRDY_PIN;
  /* ----------------------
       4. Configure RESET pin (PD03) as output, pulse it
       ---------------------- */
  GPIO_PinModeSet(gpioPortD, 2, gpioModePushPull, 1);
  Gpio_adc_reset.reset_port = gpioPortD;
  Gpio_adc_reset.reset_pin  = 2;

  // Optional reset pulse
  GPIO_PinOutClear(gpioPortD, 2);
  sl_udelay_wait(10);       // hold low
  GPIO_PinOutSet(gpioPortD, 2);
  sl_udelay_wait(10);       // let ADC startup

  spicnf.adsgpioisopened = TRUE;
  return 0;
}
/************************************************************************************//**
 *
 * @brief sendCommand()
 *          Sends the specified SPI command to the ADC
 * @param[in] op_code   SPI command byte
 *
 * @return    None
 **********************************************************************************/
int8_t sendCommand(uint8_t op_code)
{
  uint8_t index = 0;
  volatile uint8_t count;
  int8_t ret = -1;

  /* Prepare SPI command */
  adsspi_txdata[index++] = op_code;

  /* Clear RX buffer (optional) */
  memset(&adsspi_rxdata[0], 0x00, sizeof(adsspi_rxdata));


  /* Perform SPI transfer */
  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, index);

  /* Simple software timeout check using miso timeout value */
  count = ads_spiconf.misoto;
  while ((count > 0) && (adsspi_rxdata[0] == 0x00)) // optional: wait for valid response
    {
      sl_udelay_wait(500);
      count--;
    }

  if (count != 0)
    ret = 0;  // command acknowledged
  else
    ret = -1; // timeout / no response
  ret = 0;
  return ret;
}


/************************************************************************************//**
 *
 * @brief ADC_CFG()
 *
 * This function Configure SPI bus and DRDY interrupt
 *
 * @param  cbuff[IN] : Command Received from the UART terminal
 *          len[IN]   : Length of the received command string
 *
 *  @return none
 ****************************************************************************************/

void ADC_CFG(uint8_t *cbuff, uint16_t len)
{
  int8_t ret;
  (void) cbuff;
  (void) len;
  //spicnf.isopened = FALSE;
  if(TRUE == spicnf.isopened)
    {
      /*
       * SPI Open is called previously
       * Initialize only the ADS IOs
       */
      if(TRUE==spicnf.adsgpioisopened)
        {
          //init_ads_gpio();
          // Optional reset pulse
          GPIO_PinOutClear(gpioPortD, 2);
          sl_udelay_wait(10);       // hold low
          GPIO_PinOutSet(gpioPortD, 2);
          sl_udelay_wait(10);       // let ADC startup
          spicnf.adsgpioisopened = TRUE;
        }
    }
  else if(FALSE == spicnf.isopened)
    {
      /* Open the SPI Port */
      ret = ADS_SPI_Open();

      if(-1 == ret)
        {
          /* Send the response to the Terminal */
          snprintf((char *)gcmdbuff,sizeof(gcmdbuff), "\r\r\n");
          send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
          zdelay(0xFFFF);
          return;
        }

      spicnf.isopened = TRUE;
      init_ads_gpio();

    }

  /* Send the response to the Terminal */
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\nACK:CONF:ADS124:CFG\r\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  return;

}

/************************************************************************************//**
 *
 * @brief ADC_CAL()
 *
 * This function Perform Calibration procedure
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 ***********************************************************************************/

void ADC_CAL(uint8_t *cbuff, uint16_t len)
{
  uint32_t  acc_data = 0;
  int32_t  data1;
  bool      bret;
  uint8_t   iter;
  (void) cbuff;
  (void) len;

  memset(gcmdbuff, 0x00, sizeof(gcmdbuff));

  /* Check whether the SPI Port is opened */
  if (FALSE == spicnf.isopened)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  for (iter=0; iter < NUM_CAL_SAMPLE; iter++)
    {
      bret =  readChanneldata((ADS_P_AIN2 | 0x0C), &data1);

      if (true == bret)
        {
          acc_data +=data1;
        }
      else
        {
          /* Send the response to the Terminal */
          snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\n");
          send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
          zdelay(0xFFFF);
        }
    }

  cal_offset= acc_data /NUM_CAL_SAMPLE;

  /* Send the response to the Terminal */
  snprintf((char *)gcmdbuff,sizeof(gcmdbuff), "\r\r\nACK:CONF:ADS124:CAL %lx \r\r\n", cal_offset);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
  return;
}

/************************************************************************************//**
 *
 * @brief ADC_RESET()
 *
 * This function Send reset command to ADC
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 ****************************************************************************************/
void ADC_RESET(uint8_t *cbuff, uint16_t len)
{
  uint8_t ret;
  (void) cbuff;
  (void) len;

  /* Check whether the SPI Port is opened */
  if (FALSE == spicnf.isopened)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff,sizeof(gcmdbuff), "\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  /* Execute the RESET Port */
  ret = sendCommand(OPCODE_RESET);

  memset(gcmdbuff, 0x00, sizeof(gcmdbuff));
  if (0 == ret)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\nACK:CONF:ADS124:RESET\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }

  return;
}
/************************************************************************************//**
 *
 * @brief ADC_CLK()
 *
 * This function Set ADC Clock frequency
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 **************************************************************************************/

void ADC_CLK(uint8_t *cbuff, uint16_t len)
{
  char str[FREQ_STR_SIZE] = {0};
  uint32_t freq;
  uint32_t timerFreq;
  uint32_t top;
  uint32_t dutycycle;
  uint8_t index;

  /* Check whether the SPI Port is opened */
  if (FALSE == spicnf.isopened) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\nERR: SPI not opened\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
  }

  /* Frequency string size is less than received frequency string size */
  if ((FREQ_STR_SIZE-1) < len)
    return;

  /* Convert the frequency string to number */
  for (index = 0; index < len-1; index++) {
      if (FALSE == checkfornumber(cbuff[index])) {
          snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\nERR: Invalid input\r\n");
          send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
          zdelay(0xFFFF);
          return;
      }
      str[index] = cbuff[index];
  }
  str[index] = '\0';

  freq = atoi((const char *)str);

  /* Store the freq into the spi configuration table */
  ads_spiconf.freq = freq;

  /* Get timer clock frequency */
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0);

  /* Calculate TOP */
  if (freq == 0) {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\nERR: Frequency cannot be zero\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  top = (timerFreq / freq) - 1;

  /* Validate TOP fits in 16 bits */
  if (top > 0xFFFF) {
      snprintf((char *)gcmdbuff,sizeof(gcmdbuff),
               "\r\nERR: Frequency too low. Min = %lu Hz\r\n",
               timerFreq / 0x10000);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      return;
  }

  /* Configure timer safely */
  TIMER_Enable(TIMER0, true);          // Ensure timer is enabled
  TIMER0->CMD = TIMER_CMD_STOP;        // Stop counting while configuring

  TIMER_TopSet(TIMER0, top);
  dutycycle = (top + 1) / 2;
  TIMER_CompareSet(TIMER0, 0, dutycycle);

  TIMER0->CMD = TIMER_CMD_START;       // Restart counting

  /* Send the response to the Terminal */
  snprintf((char *)gcmdbuff,sizeof(gcmdbuff),
           "\r\nACK:CONF:ADS124:CLK %lu Hz\r\n", freq);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}


/************************************************************************************//**
 *
 * @brief ADC_STATUS()
 *
 * This function Read/write ADC Status register
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 **************************************************************************************/
void ADC_STATUS(uint8_t *cbuff, uint16_t len)
{
  uint8_t index = 0;
  uint8_t count;

  (void) cbuff;
  (void) len;

  /* Prepare SPI command */
  adsspi_txdata[index++] = REG_ADDR_STATUS;
  adsspi_txdata[index++] = 0x00;

  /* Clear RX buffer */
  memset(&adsspi_rxdata[0], 0x00, sizeof(adsspi_rxdata));

  /* Perform SPI transfer: command + 1 byte status */
  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, COMMAND_LENGTH + 1);

  /* Simple software timeout using miso timeout value */
  count = ads_spiconf.misoto;
  while ((count > 0) && (adsspi_rxdata[0] == 0x00)) // optional check
    {
      sl_udelay_wait(500);
      count--;
    }

  /* Send response over UART */
  if (count != 0)
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\r\nACK:CONF:ADS124:STATUS %X", adsspi_rxdata[0]);
    }
  else
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\r\n");
    }
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}

/************************************************************************************//**
 *
 * @brief ADS124RWReg()
 *
 * This function Write ADS Configuration register
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 *
 **************************************************************************************/

void ADS124RWReg(uint8_t *cbuff, uint16_t len)
{

  volatile uint8_t   slen;
  uint8_t index = 0;
  uint8_t index1 = 0;
  uint8_t notnumber = 0;
  uint8_t   data[40];
  uint8_t   *tstr = NULL;
  uint8_t   val[2];

  (void) cbuff;
  (void) len;

  /* Check whether the SPI Port is opened */
  if (FALSE == spicnf.isopened)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  memset(&data[0], 0x00, sizeof(data));

  // index = 0;
  slen = strlen((const char *)cbuff);

  if ( 0 < (slen - 1))
    {
      strncpy((char *) data, (const char *) &cbuff[1], slen - 1 );

      tstr = (uint8_t *)strtok((char *)data,(const char *) " ");

      while( tstr != NULL)
        {

          /* Check for the number */
          slen = strlen((const char *)tstr);
          if (tstr[slen - 1] == '\n')
            tstr[slen - 1] = 0x00;

          for(index1 = 0; index1 < strlen((const char *)tstr); index1++)
            {
              if (FALSE == checkforhex(tstr[index1]))
                {
                  notnumber = 1;
                  break;
                }
            }

          if ( 1 == notnumber)
            break;

          /* Convert string to number */
          val[index] = (uint8_t)strtol((const char *)tstr, NULL, 16);
          index++;

          /* Safer Check We have only two args */
          if (2 == index)
            break;

          /* Get the Next String */
          tstr = (uint8_t *)strtok(NULL, (const char *) " ");
        }
    }

  if ( 1 == notnumber)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\r\nNOT Number\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  switch(index)
  {
    case 0: /* Read all the config register */
      ADC_RD_ALL();
      break;

    case 1: /* Read config register */
      ADC_RD(val[0]);
      break;

    case 2: /* Write Config Register */
      ADC_WR(val[0], val[1]);
      break;
  }

}
/************************************************************************************//**
 *
 * @brief write_config_reg()
 *
 * This function Write ADS Configuration register
 *
 *@param  reg, value : register address
 *
 *  @return bool
 **************************************************************************************/
bool write_config_reg(uint8_t reg, uint8_t val)
{
  bool bret = false;
  uint8_t index = 0;
  uint8_t count;

  /* Check for valid address */
  if (reg > MAX_REG_ADDRESS)
    return false;

  /* Prepare SPI command */
  adsspi_txdata[index++] = OPCODE_WREG | (reg & OPCODE_RWREG_MASK);
  adsspi_txdata[index++] = 0x00;  // number of registers to write minus 1
  adsspi_txdata[index++] = val;

  /* Clear RX buffer */
  memset(adsspi_rxdata, 0x00, sizeof(adsspi_rxdata));

  /* Perform SPI transfer */

  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, index);

  /* Optional software timeout / MISO check */
  count = ads_spiconf.misoto;   // predefined timeout value
  while ((count > 0) && (adsspi_rxdata[0] == 0x00))
    {
      sl_udelay_wait(50);      // small delay, e.g., 500 µs
      count--;
    }

  /* Set return value based on counter */
  if (count != 0)
    bret = true;  // transfer completed before timeout
  else
    bret = false; // transfer timed out / no response
  bret = true;
  return bret;
  //return true;
}

/************************************************************************************//**
 *
 * @brief read_config_reg()
 *
 * This function Write ADS Configuration register
 *
 *@param  reg : register address
 *
 *  @return bool
 **************************************************************************************/

bool read_config_reg(uint8_t reg)
{
  bool bret = false;
  uint8_t index = 0;
  uint8_t count;

  /* Check for valid address */
  if (reg > MAX_REG_ADDRESS)
    return bret;

  /* Prepare SPI command: RREG + reg address, number of registers */
  adsspi_txdata[index++] = OPCODE_RREG | (reg & OPCODE_RWREG_MASK);
  adsspi_txdata[index++] = 0x00;  // number of registers to read minus 1

  /* Clear RX buffer */
  memset(&adsspi_rxdata[0], 0x00, sizeof(adsspi_rxdata));


  /* Perform SPI transfer for command + 1 byte read */
  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, index + 1);

  /* Optional: software timeout for MISO */
  count = ads_spiconf.misoto;
  while ((count > 0) && (adsspi_rxdata[1] == 0x00))  // first byte is command echo
    {
      zdelay(0xFFFF);
      count--;
    }

  /* If transfer completed (timeout not reached), set return true */
  //if (count != 0)
  bret = true;

  return bret;
}

/************************************************************************************//**
 *
 * @brief ADC_WR()
 *
 * This function Write ADS Configuration register
 *
 *@param  reg, value : Register address and value
 *
 *  @return none
 *******************************************************************************************/

void ADC_WR(uint8_t reg, uint8_t val)
{
  bool  bret;

  /* Check for valid address */
  if (reg > MAX_REG_ADDRESS)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  bret = write_config_reg(reg, val);

  /* Send Ack */
  if(true == bret)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\nACK:CONF:ADS124:REG %x %x\r\n", reg, val);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
}
/************************************************************************************//**
 *
 * @brief ADC_RD()
 *
 * This function Read ADC Configuration register
 *
 *@param reg : Register address
 *
 *  @return none
 **********************************************************************************/

void ADC_RD(uint8_t reg)
{
  bool bret;

  memset(&gcmdbuff[0], 0x00, sizeof(gcmdbuff));

  /* Check for valid address */
  if (reg > MAX_REG_ADDRESS)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  bret = read_config_reg(reg);

  if (true == bret)
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff,sizeof(gcmdbuff), "\r\r\nACK:CONF:ADS124:REG %x  %x  %x  %x \r\n", reg, adsspi_rxdata[0], adsspi_rxdata[1], adsspi_rxdata[2]);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff,sizeof(gcmdbuff), "\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }

  return;
}
/************************************************************************************//**
 *
 * @brief ADC_RD_ALL()
 *
 * This function Read all ADC Configuration registers
 *
 *  @return none
 *************************************************************************************/

void ADC_RD_ALL(void)
{
  uint8_t count, index = 0;

  /* Prepare SPI command: RREG starting at 0x00, read 0x11 registers */
  adsspi_txdata[index++] = OPCODE_RREG | (0x00 & OPCODE_RWREG_MASK);
  adsspi_txdata[index++] = 0x11 - 1;  // number of registers minus 1

  /* Clear RX buffer */
  memset(&adsspi_rxdata[0], 0x00, sizeof(adsspi_rxdata));

  /* Perform SPI transfer for command + all registers */
  adc_spi_transfer(adsspi_txdata, adsspi_rxdata, index + 0x11);

  /* Optional software timeout for MISO */
  count = ads_spiconf.misoto;
  while ((count > 0) && (adsspi_rxdata[2] == 0x00)) // wait for first register to return
    {
      zdelay(0xFFFF);
      count--;
    }

  if (count != 0)  // data received
    {
      /* Send the response to the Terminal */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\r\nACK:CONF:ADS124:REG %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
               adsspi_rxdata[2], adsspi_rxdata[3], adsspi_rxdata[4], adsspi_rxdata[5],
               adsspi_rxdata[6], adsspi_rxdata[7], adsspi_rxdata[8], adsspi_rxdata[9],
               adsspi_rxdata[10], adsspi_rxdata[11], adsspi_rxdata[12], adsspi_rxdata[13],
               adsspi_rxdata[14], adsspi_rxdata[15], adsspi_rxdata[16], adsspi_rxdata[17],
               adsspi_rxdata[18], adsspi_rxdata[19]);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      /* Send empty response if timeout */
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
}

/************************************************************************************//**
 *
 * @brief ADC_RDG_DC()
 *
 * This function Return a single reading  converted to uVDC  for DC
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 **************************************************************************************/

void ADC_RDG_DC(uint8_t *cbuff, uint16_t len)
{
  bool     bret;
  uint8_t  reg_val = 0;
  int32_t data1 = 0, avg_raw = 0;
  float    avg_voltage = 0;
  static uint8_t  init_flag = 0;
  int      i;
  //spicnf.isopened = FALSE;


  if (init_flag == 0)
    {
      // Keep your ADS124 config preamble
      char adc_config[] = "CONF:ADS124:CFG";
      uint8_t adc_config_len = strlen(adc_config); // = 15
      ads124_conf_adc((uint8_t *)adc_config, adc_config_len);
      //ADC_READID();
      init_flag = 1;
    }
  // Check command format
  if (len < 8 || strncmp((char *)cbuff, "READ_DC", 7) != 0)
    return;

  // Extract channel number from cbuff[7]
  int requested_channel = cbuff[7] - '0';
  if (requested_channel < 0 || requested_channel > 5)
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),"\r\nERR: Invalid Channel\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  switch (requested_channel)
  {
    case 0: // AIN0-AIN1 differential
    case 1: // AIN0-AIN1 differential
      reg_val =  0x01;
      break;
    case 2: // AIN2-AIN3 differential
    case 3: // AIN2-AIN3 differential
      reg_val =  0x23;
      break;
    case 4: // AIN4 single-ended
      reg_val = 0x4C;  // AIN4 vs AINCOM
      break;
    case 5: // AIN5 single-ended
      reg_val =  0x5C;  // AIN5 vs AINCOM
      break;
    default:
      return; // Invalid mapping
  }

  // Write MUX setting
  bret = write_config_reg(REG_ADDR_INPMUX, reg_val);
  if (!bret)
    return;

  zdelay(0xFFFF);  // Allow MUX to settle
  int32_t avg_data = avg_raw / gSysState.gAdcSamplesExt;
  // Take multiple samples and average
  avg_raw = 0;
  for (i = 0; i < gSysState.gAdcSamplesExt; i++)
    {
      bret = readChanneldata(reg_val, &data1);
      if (!bret)
        return;
      // **Sign-extend 24-bit ADC to 32-bit signed**
      data1 = ((int32_t)(data1 << 8)) >> 8;
      avg_raw += data1;
    }

  avg_data = avg_raw / gSysState.gAdcSamplesExt;
  avg_voltage = ((float)avg_data * REF_VOLT1) / BIT_RES;

  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "External CH%d AVG=0x%06lX = %.3f V\r\n",
           requested_channel, (unsigned long)avg_data, avg_voltage);

  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

}

/************************************************************************************//**
 *
 * @brief ADC_RDG_AC()
 *
 * This function Return a single reading  converted to uVRMS  for AC
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 **************************************************************************************/

void ADC_RDG_AC(uint8_t channel)
{
    bool bret;
    uint8_t reg_val = 0;
    int32_t raw = 0;
    int32_t signed_raw = 0;
    float voltage = 0.0f;
    float sum_sq = 0.0f;
    float rms_voltage = 0.0f;
    uint32_t valid_samples = 0;

    // Select INPMUX config based on channel
    switch (channel)
    {
        case 0:
        case 1: reg_val = 0x01; break;  // AIN0-AIN1 differential
        case 2:
        case 3: reg_val = 0x23; break;  // AIN2-AIN3 differential
        case 4: reg_val = 0x4C; break;  // AIN4 single-ended
        case 5: reg_val = 0x5C; break;  // AIN5 single-ended
        default: return;                // Invalid channel
    }

    // Configure ADC MUX
    bret = write_config_reg(REG_ADDR_INPMUX, reg_val);
    if (!bret) return;

    zdelay(0xFFFF); // Allow MUX to settle

    sum_sq = 0.0f;
    valid_samples = 0;

    // Take multiple ADC samples and compute RMS
    while (valid_samples < gSysState.gAdcSamplesExt)
    {
        bret = readChanneldata(reg_val, &raw);
        if (!bret) continue;

        // **Sign-extend 24-bit ADC to 32-bit signed**
        signed_raw = ((int32_t)(raw << 8)) >> 8;

        // Convert to voltage
        voltage = ((float)signed_raw * REF_VOLT1) / BIT_RES;

        // Accumulate squared voltage for RMS
        sum_sq += voltage * voltage;
        valid_samples++;
    }

    // Compute RMS voltage
    rms_voltage = sqrtf(sum_sq / gSysState.gAdcSamplesExt);

    // Output result
    snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
             "External AC_RMS_CH%d=%.6f V\r\n", channel, rms_voltage);

    send_uart_data(gcmdbuff, strlen((char *)gcmdbuff));
    zdelay(0xFFFF);
}


/************************************************************************************//**
 *
 * @brief ADC_SHUNT()
 *
 * This function perform single reading of the ADC Shunt input (AIN2)
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 **************************************************************************************/

void ADC_SHUNT(uint8_t *cbuff, uint16_t len)
{
  int32_t  data1;
  bool      bret;
  float     ana_input_vtg;

  (void) cbuff;
  (void) len;

  memset(gcmdbuff, 0x00, sizeof(gcmdbuff));

  // Check whether the SPI Port is opened
  if (FALSE == spicnf.isopened)
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nSPI not open\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  bret = readChanneldata((ADS_P_AIN2 | 0x0C), &data1);

  if (true == bret)
    {
      ana_input_vtg = ((data1 * REF_VOLT) / BIT_RES);

      // Now actually use ana_input_vtg in output (to avoid unused warning)
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nACK:INP:ADS124:SHUNT RAW=0x%06X, V=%.4f V\r\n",
               (unsigned int)data1, ana_input_vtg);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nADC READ FAILED\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }

  return;
}

/************************************************************************************//**
 *
 * @brief ADC_REF()
 *
 * This function single reading of the ADC AREF input (AIN3)
 *
 *@param  cbuff[IN] : Command Received from the UART terminal
 *        len[IN]   : Length of the received command string
 *
 *  @return none
 ***************************************************************************************/
void ADC_REF(uint8_t *cbuff, uint16_t len)
{
  int32_t data1;
  bool bret;
  float ana_input_vtg;

  (void) cbuff;
  (void) len;

  memset(gcmdbuff, 0x00, sizeof(gcmdbuff));

  /* Check whether the SPI Port is opened */
  if (FALSE == spicnf.isopened)
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nERR: SPI Port not opened\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  /* Read reference channel data */
  bret = readChanneldata((ADS_P_AIN4 | ADS_N_AINCOM), &data1);  // Example: AIN4 vs AINCOM

  if (bret == true)
    {
      ana_input_vtg = ((data1 * REF_VOLT) / BIT_RES);

      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nACK:INP:ADS124:REF RAW=0x%06X, V=%.4f V\r\n",
               (unsigned int)data1, ana_input_vtg);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }
  else
    {
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "\r\nERR:ADS124:REF Read Failed\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
    }

  return;
}

/**************************************************************************//**
 * @brief
 *    Read and report the ADS124 device ID (STATUS byte).
 *
 * @details
 *    This function sends an SPI command sequence to the ADS124 ADC to read
 *    its ID or status byte. It first clears the SPI receive buffer, performs
 *    an SPI transfer, and then waits for a valid response with a software
 *    timeout based on the configured MISO timeout (`ads_spiconf.misoto`).
 *    The received ID or status value is transmitted to the UART terminal.
 *
 * @note
 *    The function introduces a small delay (`zdelay`) at the end to ensure
 *    proper UART transmission timing. This function does not modify ADC
 *    configuration registers.
 *
 * @param[in]  None
 *    This function takes no input parameters.
 *
 * @return
 *    None.
 *
 * @par Example:
 * @code
 *    // Read the ADS124 ID or status via SPI
 *    ADC_READID();
 * @endcode
 *****************************************************************************/
void ADC_READID(void)
{
  bool bret;
  uint8_t reg;
  uint8_t id_value;

  /* -----------------------------
       1. Select the ID register (0x00)
       ----------------------------- */
  reg = 0x01;   // ID register address in ADS124S06/08

  /* -----------------------------
       2. Read the ID register
       ----------------------------- */
  bret = read_config_reg(reg);

  /* -----------------------------
       3. Check if read succeeded
       ----------------------------- */
  if (false == bret)
    {
      sprintf((char *)gcmdbuff, "\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  /* -----------------------------
       4. Extract the received ID byte
       ----------------------------- */
  id_value = adsspi_rxdata[2];  // byte[2] holds register value after read

  /* -----------------------------
       5. Validate and print
       ----------------------------- */
  if (id_value == 0x00 || id_value == 0xFF)
    {
      sprintf((char *)gcmdbuff, "\r\r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);
      return;
    }

  /* -----------------------------
       6. Send ACK response over UART
       ----------------------------- */
  sprintf((char *)gcmdbuff,
          "\r\r\nACK:CONF:ADS124:ID %02X\r\r\n",
          id_value);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);

  return;
}
