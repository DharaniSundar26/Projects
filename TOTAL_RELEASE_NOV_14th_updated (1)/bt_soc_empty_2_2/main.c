/***************************************************************************//**
 * @file
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "app.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else // SL_CATALOG_KERNEL_PRESENT
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT
#include "em_usart.h"
#include "process_cmd.h"
#include "em_eusart.h"
#include "em_cmu.h"
#include "common.h"
#include "sl_sleeptimer.h"
#include "em_iadc.h"
#include "em_emu.h"
#include "em_device.h"
#include "em_burtc.h"
#include "em_rmu.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#define ULFRCO_FREQ 1000   // or 32768 depending on your configuration
#define TX_BUFFER_SIZE 256
#define BURTC_IRQ_PERIOD  10000

static uint8_t txBuffer[TX_BUFFER_SIZE];

volatile uint16_t txIndex = 0;
volatile uint16_t txLength = 0;
volatile uint32_t g_rxIndex = 0;
volatile uint32_t g_receivedLength = 0;

bool Received_First_ch = false;
bool rxDataReady = false;
uint8_t rxBuffer[RX_BUFFER_SIZE];

/* Initial Sleep Time Counter for 3 seconds in EM4 Mode  */
power_mode pwr ={
    .Sleep_Counter = 0,
    .em2_wakeup_flag = false,
};

/**************************************************************************//**
 * @brief
 *    Initialize USART2 module for communication
 *****************************************************************************/
void initDebugUART(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(CONSOLE_UART_CLOCK, true);

  // Configure pins
  GPIO_PinModeSet(CONSOLE_UART_TX_PORT, CONSOLE_UART_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(CONSOLE_UART_RX_PORT, CONSOLE_UART_RX_PIN, gpioModeInput, 0);

  // Default init @ 115200
  EUSART_UartInit_TypeDef init = EUSART_UART_INIT_DEFAULT_HF;
  init.baudrate = CONSOLE_UART_BAUDRATE;
  EUSART_UartInitHf(CONSOLE_UART, &init);

  // Route pins
  GPIO->EUSARTROUTE[CONSOLE_UART_INDEX].TXROUTE = ((uint32_t)CONSOLE_UART_TX_PORT << _GPIO_EUSART_TXROUTE_PORT_SHIFT)
                                                                                                                                           | (CONSOLE_UART_TX_PIN << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[CONSOLE_UART_INDEX].RXROUTE = ((uint32_t)CONSOLE_UART_RX_PORT << _GPIO_EUSART_RXROUTE_PORT_SHIFT)
                                                                                                                                           | (CONSOLE_UART_RX_PIN << _GPIO_EUSART_RXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[CONSOLE_UART_INDEX].ROUTEEN = GPIO_EUSART_ROUTEEN_TXPEN | GPIO_EUSART_ROUTEEN_RXPEN;

  // Enable interrupts
  NVIC_EnableIRQ(CONSOLE_UART_RX_IRQ);
  NVIC_EnableIRQ(CONSOLE_UART_TX_IRQ);

  EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL);

}

/**************************************************************************//**
 * @brief
 *    USART2 RX interrupt service routine
 *
 * @details
 *    Keep receiving data while there is still data left in the hardware RX buffer.
 *    Store incoming data into rxBuffer and set rxDataReady when a linefeed '\n' is
 *    sent or if there is no more room in the buffer.
 *****************************************************************************/
void EUSART1_RX_IRQHandler(void)
{
  if (EUSART_IntGet(CONSOLE_UART) & EUSART_IF_RXFL) {
      char c = EUSART_Rx(CONSOLE_UART);
      if(c == '#')
        {
          Received_First_ch = true;
          EUSART_IntClear(CONSOLE_UART, EUSART_IF_RXFL);
          return;
        }
      if(c == '$')
        {
          Received_First_ch = false;
          EUSART_IntClear(CONSOLE_UART, EUSART_IF_RXFL);
          return;
        }
      if (c == '\r' || c == '\n') {
          rxBuffer[g_rxIndex] = '\0';
          if (g_rxIndex > 0) {
              rxDataReady = 1;
              g_receivedLength = g_rxIndex;
          }
          g_rxIndex = 0;
      }
      else if (c == 0x08 && g_rxIndex > 0) {
          g_rxIndex--; // backspace
      }
      else if (g_rxIndex < RX_BUFFER_SIZE - 1) {
          if(Received_First_ch == true)
            rxBuffer[g_rxIndex++] = c;
      }

      EUSART_IntClear(CONSOLE_UART, EUSART_IF_RXFL);
  }
}
/**************************************************************************//**
 * @brief
 *    USART2 TX interrupt service routine
 *
 * @details
 *    Keep sending data while there is still data left in the hardware TX buffer.
 *****************************************************************************/
void EUSART1_TX_IRQHandler(void)
{
  uint32_t flags = EUSART_IntGet(CONSOLE_UART);
  EUSART_IntClear(CONSOLE_UART, flags);

  if ((flags & EUSART_IF_TXFL) && txIndex < txLength) {
      EUSART_Tx(CONSOLE_UART, txBuffer[txIndex++]);
  }
  else {
      txIndex = 0;
      txLength = 0;
      EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_TXFL);
  }
}
/**************************************************************************//**
 * @brief
 *    Send UART data
 *
 * @details
 *    load UART txbuffer and enable tx interrupt to transfer data
 *
 * @param[in] buff
 *   A buffer point to data to be transfer
 *
 * @param[in] len
 *   Length of the data buffer to be send
 *
 * @return
 *   None
 ******************************************************************************/
void send_uart_data(const uint8_t *buff, size_t len)
{
  if (!buff || len == 0) return;
  if (len > TX_BUFFER_SIZE) len = TX_BUFFER_SIZE;

  memcpy(txBuffer, buff, len);
  txLength = len;
  txIndex  = 0;

  // Send first byte manually
  EUSART_Tx(CONSOLE_UART, txBuffer[txIndex++]);

  // Enable TX interrupt for the rest
  EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_TXFL);
}

/**************************************************************************//**
 * @brief
 *    Busy-wait loop delay
 *
 * @details
 *    Simple volatile loop for short delays.
 *    Approximate delay depends on CPU frequency and compiler optimization.
 *
 * @param[in] dly
 *    Loop count for delay.
 *****************************************************************************/
void zdelay(uint32_t dly)
{
  volatile uint32_t index = dly;

  while (index) {
      index--;
      __NOP();  // No-operation to prevent loop from being optimized out
  }
}
/**************************************************************************//**
 * @brief
 *    Tiny millisecond delay
 *
 * @details
 *    Approximate 1 ms delay per 3200 iterations @ ~32 MHz system clock.
 *
 * @param[in] ms
 *    Number of milliseconds to delay.
 *****************************************************************************/
void delay_ms(uint32_t ms)
{
  for (volatile uint32_t i = 0; i < ms * 3200u; i++) {
      __NOP();  // Prevent optimization
  }
}
/**************************************************************************//**
 * @brief
 *    Initialize GPIO pins
 *
 * @details
 *    Configures all required GPIO pins:
 *      - Control pins: MEM_PWR, SWOUT, GPS_EN, GPS_RESET
 *      - Misc pins: PC0–PC9, PD7
 *      - SPI Flash pins: SCK, MOSI, MISO, CS
 *    Pins are set as push-pull output or input with default logic levels.
 *****************************************************************************/
void gpio_init(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Control pins (outputs, initially low)
  GPIO_PinModeSet(GPIO_MEM_PWR_PORT, GPIO_MEM_PWR_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_SWOUT_PORT, GPIO_SWOUT_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_GPS_EN_PORT, GPIO_GPS_EN_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(GPIO_GPS_RESET_PORT, GPIO_GPS_RESET_PIN, gpioModePushPull, 0);

  // Port C pins (general purpose outputs)
  GPIO_PinModeSet(gpioPortC, 0, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 1);  // PC1 default HIGH
  GPIO_PinModeSet(gpioPortC, 2, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 4, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 5, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 6, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 7, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 8, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 9, gpioModePushPull, 0);

  // Port D pin
  GPIO_PinModeSet(gpioPortD, 7, gpioModePushPull, 0);

  // SPI Flash pins
  GPIO_PinModeSet(FLASH_SCK_PORT, FLASH_SCK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(FLASH_MOSI_PORT, FLASH_MOSI_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(FLASH_MISO_PORT, FLASH_MISO_PIN, gpioModeInput, 0); // Input
  GPIO_PinModeSet(FLASH_CS_PORT, FLASH_CS_PIN, gpioModePushPull, 1);  // Default HIGH
}

void EUSART2_RX_IRQHandler(void)
{

  if (EUSART_IntGet(EUSART2) & EUSART_IF_RXFL) {
      char c = EUSART_Rx(EUSART2);
      // Forward GPS data to debug UART
      EUSART_Tx(CONSOLE_UART, c);
      EUSART_IntClear(EUSART2, EUSART_IF_RXFL);
  }
}

/**************************************************************************//**
 * @brief
 *    Enable GPS RX interrupts
 *
 * @details
 *    - Clears any pending RX flags.
 *    - Enables RX interrupt for EUSART2.
 *    - Enables the NVIC interrupt line.
 *****************************************************************************/
void EUSART2_enableGPSRx(void)
{
  EUSART_IntClear(EUSART2, EUSART_IF_RXFL);   // Clear pending RX interrupt
  EUSART_IntEnable(EUSART2, EUSART_IF_RXFL);  // Enable RX interrupt
  NVIC_EnableIRQ(EUSART2_RX_IRQn);            // Enable NVIC IRQ
}

/**************************************************************************//**
 * @brief
 *    Disable GPS RX interrupts
 *
 * @details
 *    - Disables RX interrupt for EUSART2.
 *    - Disables the NVIC interrupt line.
 *****************************************************************************/
void EUSART2_disableGPSRx(void)
{
  NVIC_DisableIRQ(EUSART2_RX_IRQn);           // Disable NVIC IRQ
  EUSART_IntDisable(EUSART2, EUSART_IEN_RXFL); // Disable EUSART2 RX interrupt
}
/**************************************************************************//**
 * @brief
 *    Initialize EUSART2 for GPS communication
 *
 * @details
 *    - Enables clocks for GPIO and EUSART2.
 *    - Configures TX pin as push-pull output and RX as input.
 *    - Initializes EUSART2 for 9600 baud, 8N1.
 *    - Routes TX/RX to specified GPIO pins.
 *    - Enables RX and TX interrupts in NVIC.
 *****************************************************************************/
void initEUSART2_GPS(void)
{
  // Enable clocks
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_EUSART2, true);

  // Configure TX (push-pull) and RX (input) pins
  GPIO_PinModeSet(EUSART2_TX_PORT, EUSART2_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(EUSART2_RX_PORT, EUSART2_RX_PIN, gpioModeInput, 0);

  // Initialize UART parameters
  EUSART_UartInit_TypeDef init = EUSART_UART_INIT_DEFAULT_HF;
  init.baudrate = 9600;                     // GPS default baudrate
  EUSART_UartInitHf(EUSART2, &init);        // Apply configuration

  // Route pins to GPIO
  GPIO->EUSARTROUTE[2].TXROUTE = ((uint32_t)gpioPortC << _GPIO_EUSART_TXROUTE_PORT_SHIFT)
                                                                   | (10 << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[2].RXROUTE = ((uint32_t)gpioPortC << _GPIO_EUSART_RXROUTE_PORT_SHIFT)
                                                                   | (11 << _GPIO_EUSART_RXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[2].ROUTEEN = GPIO_EUSART_ROUTEEN_TXPEN | GPIO_EUSART_ROUTEEN_RXPEN;

  // Enable interrupts in NVIC
  NVIC_EnableIRQ(EUSART2_RX_IRQn);
  NVIC_EnableIRQ(EUSART2_TX_IRQn);

  // Enable RX FIFO interrupts
  EUSART_IntEnable(EUSART2, EUSART_IEN_RXFL);
}

/**************************************************************************//**
 * @brief
 *    Monitor the keypad once
 *
 * @details
 *    - Enables the keypad state in gSysState.
 *    - Continuously polls the keypad for key presses.
 *    - Processes system actions while waiting.
 *    - Listens for the "KEYPAD=OFF" UART command to exit the loop.
 *    - Sends debug messages when keypad is enabled/disabled.
 *****************************************************************************/
void gpio_monitor_once(void)
{
  const char *keypad_off = "KEYPAD=OFF";  // Command to disable keypad

  // Set system state to indicate keypad is active
  gSysState.keypad = true;

  // Notify user via UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\nKEYPAD Enabled. Press keys...\r\n");
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);  // Small delay for UART flush

  // Polling loop for key presses
  while (1)
    {
      // Scan keypad and handle key presses
      process_keypad_loop();

      // Process periodic system actions (e.g., BLE stack, timers)
      sl_system_process_action();

      // Check if UART received data
      if (rxDataReady)
        {
          // Temporarily disable UART RX/TX interrupts while parsing
          EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXFL);

          // If "KEYPAD=OFF" command received, exit loop
          if (strncmp((const char *)rxBuffer, keypad_off, strlen(keypad_off)) == 0)
            {
              rxDataReady = 0;
              memset((void *)rxBuffer, 0, sizeof(rxBuffer));
              break;
            }

          // Not KEYPAD=OFF → reset buffer and re-enable interrupts
          rxDataReady = 0;
          memset((void *)rxBuffer, 0, sizeof(rxBuffer));
          EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXFL);
        }
    }

  // Keypad monitoring finished → update system state
  gSysState.keypad = false;

  // Notify user via UART
  int n = snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
                   "\r\nKEYPAD Disabled.\r\n");
  if (n > 0)
    {
      send_uart_data((const uint8_t *)gcmdbuff, (size_t)n);
    }

  zdelay(0xFFFF);  // Delay for UART transmission completion
}

/**************************************************************************//**
 * @brief
 *    BURTC interrupt handler
 *
 * @details
 *    This function handles the BURTC compare interrupt.
 *    It clears the interrupt flag to acknowledge it.
 *****************************************************************************/
void BURTC_IRQHandler(void)
{
  // Clear only the compare interrupt flag
  BURTC_IntClear(BURTC_IF_COMP);
}
/**************************************************************************//**
 * @brief
 *    Initialize BURTC for periodic interrupts and EM4 wakeup
 *
 * @details
 *    Configures the Backup Real-Time Counter (BURTC) to:
 *      - Use ULFRCO as clock source (ultra-low frequency RC)
 *      - Reset counter on compare match
 *      - Wake the device from EM4 when compare match occurs
 *****************************************************************************/
void initBURTC(void)
{
  // Select ULFRCO for EM4 group A clock
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_ULFRCO);

  // Enable clocks for BURTC and backup RAM
  CMU_ClockEnable(cmuClock_BURTC, true);
  CMU_ClockEnable(cmuClock_BURAM, true);

  // Initialize BURTC structure with default settings
  BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;
  burtcInit.compare0Top = true; // Reset counter at compare match
  burtcInit.em4comp     = true; // Allow EM4 wakeup on compare match

  // Apply BURTC initialization
  BURTC_Init(&burtcInit);

  // Reset BURTC counter to 0
  BURTC_CounterReset();

  // Set compare value to desired period
  BURTC_CompareSet(0, BURTC_IRQ_PERIOD);

  // Enable compare interrupt
  BURTC_IntEnable(BURTC_IEN_COMP);

  // Enable NVIC interrupt for BURTC
  NVIC_EnableIRQ(BURTC_IRQn);

  // Enable BURTC
  BURTC_Enable(true);
}
/**************************************************************************//**
 * @brief
 *    Check RMU reset cause and track EM4 wakeups
 *
 * @details
 *    - Reads the Reset Management Unit (RMU) cause register
 *    - Clears RMU cause flags
 *    - Updates backup RAM counter for EM4 wakeups
 *    - Sends debug messages via UART
 *****************************************************************************/
void checkResetCause(void)
{
  // Read the reset cause register
  uint32_t cause = RMU_ResetCauseGet();

  // Clear all reset cause flags
  RMU_ResetCauseClear();

  if (cause & EMU_RSTCAUSE_EM4)
    {
      // Notify via UART
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "-- RSTCAUSE = EM4 wakeup \r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);

      // Increment EM4 wakeup counter in backup RAM
      BURAM->RET[0].REG += 1;

      // Restore the last user-defined Sleep_Counter from backup RAM
      pwr.Sleep_Counter = BURAM->RET[1].REG;
      // Print total number of EM4 wakeups
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
               "-- Number of EM4 wakeups = %ld \r\n", BURAM->RET[0].REG);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);

      BURTC_CounterReset(); // reset BURTC counter to wait full ~3 sec before EM4 wakeup
      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),  "-- BURTC counter reset \r\n");
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);

      snprintf((char *)gcmdbuff, sizeof(gcmdbuff),  "EM4  sleeptimer = %u\r\n", (unsigned int)pwr.Sleep_Counter);
      send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
      zdelay(0xFFFF);

      BURAM->RET[1].REG = 0;

    }
  else
    {
      BURAM->RET[0].REG = 0;
      BURAM->RET[1].REG = 0;
    }
}
int main(void)
{
  sl_system_init();
  // ---- UART Init ----
  initDebugUART();        // Debug/command UART
  initEUSART2_GPS();      // GPS UART

  // ---- Peripherals ----
  keypad_init();          //KEYPAD
  flash_power_on();       // Enable Flash power

  // ---- App Init ----
  app_init();
  app_init_runtime();
  gpio_init();
  GPIO_PinModeSet(ANALOG_ENABLE_PORT, ANALOG_ENABLE_PIN, gpioModePushPull, 1);
  /* Turn Off All Realys */
  All_relay_off();
  GPIO_PinOutClear(ANALOG_ENABLE_PORT,ANALOG_ENABLE_PIN);
  initIADC();

  initBURTC();
  EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
  EMU_EM4Init(&em4Init);
  BURTC_IntEnable(BURTC_IEN_COMP);    // compare match
  NVIC_EnableIRQ(BURTC_IRQn);
  BURTC_Enable(true);
  checkResetCause();
  /* Main loop */
  while (1) {
      //process_keypad_loop();
      // ---- BLE + System ----
      sl_system_process_action();
      app_process_action();

      // ---- UART Command Processing ----
      if (rxDataReady) {
          // Disable UART RX/TX interrupt while parsing
          EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);

          process_cmd((uint8_t *)&rxBuffer[0], g_receivedLength);

          // Reset buffer
          memset((void *)&rxBuffer[0], 0x00, sizeof(rxBuffer));
          g_rxIndex = 0;
          rxDataReady = 0;

          // Re-enable UART interrupts
          EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXFL);
      }
      //  Handle EM2 wakeup flag
      if (pwr.em2_wakeup_flag) {
          pwr.em2_wakeup_flag = false;
          send_uart_data((const uint8_t *)"Woke up from EM2\r\n", sizeof("Woke up from EM2\r\n") - 1);
          zdelay(0xFFFF);
      }
  }

}
