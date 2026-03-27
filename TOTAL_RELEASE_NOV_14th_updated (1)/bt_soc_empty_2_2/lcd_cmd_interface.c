/*
 * lcd_cmd_interface.c
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */
#include "em_device.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "common.h"
#include "sl_udelay.h"
#include "sl_system_process_action.h"
#include <stdint.h>
#include "em_eusart.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "em_timer.h"

uint8_t lcdDutyCycle = 30;  // percent

// 5x7 font table for capital letters 'A' to 'Z'
const uint8_t font5x7[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}  // 'Z'
};

const char *lcd_test_off = "LCD_TEST=OFF";

/*********************************************************************//**
 * @brief  Initialize the GPIO pins used for the LCD.
 *
 * This function enables the GPIO peripheral clock and configures all
 * necessary pins for interfacing with the LCD, including DC, RESET,
 * VCC enable, and SPI pins (SCK, MOSI, MISO).
 **************************************************************************/
void LCD_GPIO_Init(void) {
  /* Enable clock for the GPIO peripheral */
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* Configure Data/Command (DC) pin as push-pull output, initially low */
  GPIO_PinModeSet(LCD_DC_PORT, LCD_DC_PIN, gpioModePushPull, 0);

  /* Configure RESET pin as push-pull output, initially high */
  GPIO_PinModeSet(LCD_RESET_PORT, LCD_RESET_PIN, gpioModePushPull, 1);

  /* Configure VCC enable pin as push-pull output, initially high */
  GPIO_PinModeSet(LCD_VCC_EN_PORT, LCD_VCC_EN_PIN, gpioModePushPull, 1);

  /* Configure SPI Clock (SCK) pin as push-pull output, initially low */
  GPIO_PinModeSet(LCD_SCK_PORT, LCD_SCK_PIN, gpioModePushPull, 0);

  /* Configure SPI Master Out Slave In (MOSI) pin as push-pull output, initially low */
  GPIO_PinModeSet(LCD_MOSI_PORT, LCD_MOSI_PIN, gpioModePushPull, 0);

  /* Configure SPI Master In Slave Out (MISO) pin as input */
  GPIO_PinModeSet(LCD_MISO_PORT, LCD_MISO_PIN, gpioModeInput, 0);
  /* Configure the PWM GPIO pin (PD10)  */
  GPIO_PinModeSet(LCD_PWM_PORT, LCD_PWM_PIN, gpioModePushPull, 0);

}

/*********************************************************************//**
 * @brief  Transmit and receive a single byte over SPI using USART2.
 *
 * This function performs a synchronous SPI transfer using the USART
 * peripheral in SPI mode. It sends a byte and waits for a response.
 *
 * @param  data  The byte to transmit.
 * @retval uint8_t  The byte received from the LCD.
 *************************************************************************/
static inline uint8_t LCD_SPI_TxRx(uint8_t data) {
  /* Wait until the TX buffer is empty */
  while (!(USART2->STATUS & USART_STATUS_TXBL));

  /* Send the byte over USART2 */
  USART_Tx(USART2, data);

  /* Wait until a byte has been received */
  while (!(USART2->STATUS & USART_STATUS_RXDATAV));

  /* Return the received byte */
  return (uint8_t)USART_Rx(USART2);
}


/*********************************************************************//**
 * @brief  Initialize SPI communication for the LCD using USART2.
 *
 * This function configures USART2 in synchronous SPI mode, routes the
 * MOSI, MISO, and SCK pins to the correct GPIO ports, and sets up
 * the SPI parameters (baud rate, master mode, clock polarity/phase, MSB first).
 ***************************************************************************/
void LCD_SPI_Init(void) {
  /* Enable clock for USART2 peripheral */
  CMU_ClockEnable(cmuClock_USART2, true);

  /* Route TX (MOSI) pin */
  GPIO->USARTROUTE[2].TXROUTE  = (LCD_MOSI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
                                                       | (LCD_MOSI_PIN  << _GPIO_USART_TXROUTE_PIN_SHIFT);

  /* Route RX (MISO) pin */
  GPIO->USARTROUTE[2].RXROUTE  = (LCD_MISO_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                                       | (LCD_MISO_PIN  << _GPIO_USART_RXROUTE_PIN_SHIFT);

  /* Route CLK (SCK) pin */
  GPIO->USARTROUTE[2].CLKROUTE = (LCD_SCK_PORT  << _GPIO_USART_CLKROUTE_PORT_SHIFT)
                                                       | (LCD_SCK_PIN   << _GPIO_USART_CLKROUTE_PIN_SHIFT);

  /* Enable TX, RX, and CLK pins */
  GPIO->USARTROUTE[2].ROUTEEN  = GPIO_USART_ROUTEEN_TXPEN |
      GPIO_USART_ROUTEEN_RXPEN |
      GPIO_USART_ROUTEEN_CLKPEN;

  /* Create default synchronous USART configuration */
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

  /* Set SPI baud rate to 10 MHz */
  init.baudrate  = 10000000;

  /* Transmit MSB first */
  init.msbf      = true;

  /* Configure as master */
  init.master    = true;

  /* SPI Mode 0: CPOL=0, CPHA=0 */
  init.clockMode = usartClockMode0;

  /* Initialize USART2 with the above configuration */
  USART_InitSync(USART2, &init);
}

/*********************************************************************//**
 * @brief  Send a 16-bit command to the LCD.
 *
 * This function sets the DC pin low to indicate a command, then
 * transmits the high byte followed by the low byte over SPI.
 *
 * @param  cmd  16-bit command to send
 *************************************************************************/
void LCD_WriteCommand(uint16_t cmd) {
  /* Set DC pin low for command */
  GPIO_PinOutClear(LCD_DC_PORT, LCD_DC_PIN);

  /* Send high byte */
  LCD_SPI_TxRx((cmd >> 8) & 0xFF);

  /* Send low byte */
  LCD_SPI_TxRx(cmd & 0xFF);
}

/*********************************************************************//**
 * @brief  Send 16-bit data to the LCD.
 *
 * This function sets the DC pin high to indicate data, then
 * transmits the high byte followed by the low byte over SPI.
 *
 * @param  data  16-bit data to send
 ***********************************************************************/
void LCD_WriteData(uint16_t data) {
  /* Set DC pin high for data */
  GPIO_PinOutSet(LCD_DC_PORT, LCD_DC_PIN);

  /* Send high byte */
  LCD_SPI_TxRx((data >> 8) & 0xFF);

  /* Send low byte */
  LCD_SPI_TxRx(data & 0xFF);
}
/*********************************************************************//**
 * @brief  Perform hardware reset of the LCD.
 *
 * This function toggles the LCD RESET pin according to the
 * required timing sequence to ensure the controller is properly reset.
 * It includes precise microsecond delays for correct initialization.
 *************************************************************************/
void LCD_RST(void) {
  /* Set RESET pin high (idle state) */
  GPIO_PinOutSet(LCD_RESET_PORT, LCD_RESET_PIN);

  /* Wait for 6 ms before asserting reset */
  sl_udelay_wait(6000);

  /* Pull RESET pin low to start the reset pulse */
  GPIO_PinOutClear(LCD_RESET_PORT, LCD_RESET_PIN);

  /* Hold reset low for 30 ms */
  sl_udelay_wait(30000);

  /* Release reset by setting RESET pin high */
  GPIO_PinOutSet(LCD_RESET_PORT, LCD_RESET_PIN);

  /* Wait 60 ms for LCD to initialize internal circuits */
  sl_udelay_wait(60000);

  /* Additional 60 ms delay to ensure full readiness */
  sl_udelay_wait(60000);
}

/*********************************************************************//**
 * @brief  Execute the SSD2119 LCD power-on initialization sequence.
 *
 * This function writes a series of commands and data to the LCD controller
 * to configure power, oscillator, and display control registers.
 * Delays are inserted to allow the internal power circuitry to stabilize.
 ************************************************************************/
void SSD2119_PWR_ON_SEQUENCE(void) {
  /* Set Display Control Register to 0x0008 as part of initialization */
  LCD_WriteCommand(DISP_CTRL);
  LCD_WriteData(0x0008);

  /* Start the internal oscillator (register OSC_START = 0x0001) */
  LCD_WriteCommand(OSC_START);
  LCD_WriteData(0x0001);

  /* Update Display Control Register to 0x0023 to enable more features */
  LCD_WriteCommand(DISP_CTRL);
  LCD_WriteData(0x0023);

  /* Set Sleep Mode register to 0x0000 (default/reset) */
  LCD_WriteCommand(SLEEP_MODE);
  LCD_WriteData(0x0000);

  /* Delay loop: 8 × 60 ms = 480 ms for power stabilization */
  for (int i = 0; i < 8; i++) {
      sl_udelay_wait(60000);
  }

  /* Finalize Display Control Register to 0x0033 to turn the display fully on */
  LCD_WriteCommand(DISP_CTRL);
  LCD_WriteData(0x0033);
}

/*********************************************************************//**
 * @brief  Initialize the SSD2119 LCD controller via SPI.
 *
 * This function sends a sequence of commands and data to configure
 * the internal registers of the SSD2119 controller for 3.5" LCD operation.
 * It sets clock, driver control, power control, and address ranges.
 ************************************************************************/
void SSD2119_35U_SPI_INIT(void) {
  /* Sleep Out / Start Oscillator: entry mode 0x6230 sets up internal clocks */
  LCD_WriteCommand(ENTRY_MODE);
  LCD_WriteData(0x6230);

  /* Horizontal Porch: sets front/back porch for column timing */
  LCD_WriteCommand(H_PORCH);
  LCD_WriteData(0x00FF);

  /* Vertical Porch: sets top/bottom porch for row timing */
  LCD_WriteCommand(V_PORCH);
  LCD_WriteData(0x00FF);

  /* LCD Driver Control: e.g., scanning direction, driver configuration */
  LCD_WriteCommand(LCD_DRIVER);
  LCD_WriteData(0x0600);

  /* Power Control 1: sets voltage and initial power configuration */
  LCD_WriteCommand(PWR_CTRL_1);
  LCD_WriteData(0xAAAE);

  /* Driver Output Control: defines display scan lines and direction */
  LCD_WriteCommand(DRIVER_OUTPUT);
  LCD_WriteData(0x32EF);

  /* Gate Scan Start Position: typically 0 */
  LCD_WriteCommand(GATE_SCAN_POS);
  LCD_WriteData(0x0000);

  /* Display Cycle Control: timing for frame rate and scan cycles */
  LCD_WriteCommand(CYCLE_CTRL);
  LCD_WriteData(0x5208);

  /* Power Control 2: additional voltage/power settings */
  LCD_WriteCommand(PWR_CTRL_2);
  LCD_WriteData(0x0005);

  /* Power Control 3: extra configuration for voltage/current */
  LCD_WriteCommand(PWR_CTRL_3);
  LCD_WriteData(0x000D);

  /* Power Control 4: further power settings */
  LCD_WriteCommand(PWR_CTRL_4);
  LCD_WriteData(0x2400);

  /* Power Control 5: final power configuration */
  LCD_WriteCommand(PWR_CTRL_5);
  LCD_WriteData(0x00AC);

  /* Vertical Address End: sets maximum row address */
  LCD_WriteCommand(V_ADDR);
  LCD_WriteData(0x00EF);

  /* Horizontal Start Address */
  LCD_WriteCommand(H_STR_ADDR);
  LCD_WriteData(0x013F);

  /* Horizontal End Address */
  LCD_WriteCommand(H_END_ADDR);
  LCD_WriteData(0x0000);

  /* Set initial RAM X address for GRAM writes */
  LCD_WriteCommand(RAM_XADDR);
  LCD_WriteData(0x0000);

  /* Set initial RAM Y address for GRAM writes */
  LCD_WriteCommand(RAM_YADDR);
  LCD_WriteData(0x0000);
}

/*********************************************************************//**
 * @brief  Set the cursor position in the LCD GRAM.
 *
 * @param  x  Row coordinate (Y-address)
 * @param  y  Column coordinate (X-address)
 *
 * This function sets the internal RAM X and Y addresses so that
 * subsequent pixel data writes affect the desired location.
 **************************************************************************/
void LCD_SetCursor(uint16_t x, uint16_t y) {
  /* Set RAM X address (column) */
  LCD_WriteCommand(RAM_XADDR);
  LCD_WriteData(y);

  /* Set RAM Y address (row) */
  LCD_WriteCommand(RAM_YADDR);
  LCD_WriteData(x);

  /* Prepare GRAM for pixel data write */
  LCD_WriteCommand(GRAM_WRITE);
}
/*********************************************************************//**
 * @brief  Write a single pixel color to the LCD GRAM.
 *
 * @param  color  16-bit color value to write
 *
 * This function writes a single pixel to the current GRAM address.
 * The GRAM address should be set beforehand using LCD_SetCursor or LCD_SetArea.
 *************************************************************************/
void LCD_Write_Color(uint16_t color) {
  /* Prepare GRAM for writing */
  LCD_WriteCommand(GRAM_WRITE);

  /* Write the 16-bit pixel color */
  LCD_WriteData(color);
}

/*********************************************************************//**
 * @brief  Set a rectangular drawing window in the LCD GRAM.
 *
 * @param  x0  Starting column (X) address
 * @param  y0  Starting row (Y) address
 * @param  x1  Ending column (X) address
 * @param  y1  Ending row (Y) address
 *
 * After calling this function, any pixel data written will
 * fill the defined rectangular window. Useful for block fills
 * or drawing images.
 *************************************************************************/
void LCD_SetArea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  /* Set horizontal RAM address range (start and end) */
  LCD_WriteCommand(0x44);
  LCD_WriteData((x1 << 8) | (x0 & 0xFF));

  /* Set vertical RAM start address */
  LCD_WriteCommand(0x45);
  LCD_WriteData(y0);

  /* Set vertical RAM end address */
  LCD_WriteCommand(0x46);
  LCD_WriteData(y1);

  /* Set GRAM X address (current write position) */
  LCD_WriteCommand(0x4E);
  LCD_WriteData(x0);

  /* Set GRAM Y address (current write position) */
  LCD_WriteCommand(0x4F);
  LCD_WriteData(y0);

  /* Prepare to write pixel data to GRAM */
  LCD_WriteCommand(0x22);
}
/*********************************************************************//**
 * @brief  Draw vertical color bars across the full LCD screen.
 *
 * This function fills the LCD with 8 vertical bars of predefined colors.
 * The width of each bar is calculated to evenly divide the screen width.
 * The last bar adjusts its width to fill any remaining pixels.
 * Incoming UART commands (LCD_TEST=OFF, LCD_BACK=, DISP_VCC=OFF) can interrupt
 * the drawing process to stop animation or perform other actions.
 ***********************************************************************/
void LCD_Draw_ColorBars_Vertical_Fast(void) {
  /* Define color palette for vertical bars */
  uint16_t colors[] = {
      COLOR_RED, COLOR_GREEN, COLOR_BLUE,
      COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
      COLOR_WHITE, COLOR_BLACK
  };

  uint8_t numBars   = sizeof(colors) / sizeof(colors[0]);  /* Number of color bars */
  uint16_t barWidth = LCD_WIDTH / numBars;                 /* Width of each bar in pixels */

  /* Set drawing window to cover the full screen */
  LCD_SetArea(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

  /* Loop through each color bar */
  for (uint8_t c = 0; c < numBars; c++) {
      uint16_t color = colors[c];  /* Current bar color */
      uint16_t width = (c == numBars - 1)
                                                 ? (LCD_WIDTH - (barWidth * (numBars - 1)))  /* Adjust last bar */
                                                     : barWidth;

      /* Draw each vertical column of the bar */
      for (uint16_t x = 0; x < width; x++) {
          for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
              LCD_WriteData(color);  /* Write pixel to GRAM */

              /* Check if UART data is ready to interrupt */
              if (rxDataReady) {
                  EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);

                  /* Stop animation if command matches */
                  if (strncmp((const char *)rxBuffer, lcd_test_off, strlen(lcd_test_off)) == 0 ||
                      strncmp((const char *)rxBuffer, "LCD_BACK=", 9) == 0 ||
                      strncmp((const char *)rxBuffer, "DISP_VCC=OFF", 11) == 0) {
                      break;
                  }
                  /* Clear UART buffer and re-enable interrupts */
                  rxDataReady = 0;
                  memset((void *)rxBuffer, 0, sizeof(rxBuffer));
                  EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);
              }
          }
      }
  }
}

/*********************************************************************//**
 * @brief  Continuously test LCD with color bars and solid colors.
 *
 * This function repeatedly draws vertical and horizontal color bars,
 * clears the screen to black, and checks for UART commands to stop
 * the test or adjust LCD settings.
 **********************************************************************/
void LCD_Testing_Bars(void) {
  while (1) {
      /* Draw vertical color bars */
      LCD_Draw_ColorBars_Vertical_Fast();
      sl_udelay_wait(10000);  /* 10 ms delay */

      /* Clear screen to black */
      LCD_Draw_Single_Color(COLOR_BLACK);
      sl_udelay_wait(10000);  /* 10 ms delay */

      /* Draw horizontal bars using cursor (function assumed implemented elsewhere) */
      LCD_Draw_ColorBars_Horizontal_Cursor();
      sl_udelay_wait(10000);  /* 10 ms delay */

      /* Clear screen to black again */
      LCD_Draw_Single_Color(COLOR_BLACK);
      sl_udelay_wait(10000);  /* 10 ms delay */

      /* Run system process action (e.g., background tasks) */
      sl_system_process_action();

      /* Check for incoming UART commands */
      if (rxDataReady) {
          EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);

          /* Stop test if LCD_TEST=OFF received */
          if (strncmp((const char *)rxBuffer, lcd_test_off, strlen(lcd_test_off)) == 0) {
              rxDataReady = 0;
              lcd_test_off_cmd(NULL, 0);
              memset((void *)rxBuffer, 0, sizeof(rxBuffer));
              break;
          }
          /* Handle backlight adjustment command */
          else if (strncmp((const char *)rxBuffer, "LCD_BACK=", 9) == 0) {
              lcd_back_cmd(rxBuffer,0);
          }
          /* Handle display VCC off command */
          else if (strncmp((const char *)rxBuffer, "DISP_VCC=OFF", 11) == 0) {
              rxDataReady = 0;
              disp_vcc_off_cmd(NULL, 0);
              memset((void *)rxBuffer, 0, sizeof(rxBuffer));
              break;
          }

          /* Clear UART buffer and re-enable interrupts */
          rxDataReady = 0;
          memset((void *)rxBuffer, 0, sizeof(rxBuffer));
          EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);
      }
  }
}

/*********************************************************************//**
 * @brief  Fill the entire LCD screen with a single color.
 *
 * @param  color  16-bit color value to fill the screen.
 *
 * @note   UART commands (LCD_TEST=OFF, LCD_BACK=, DISP_VCC=OFF) can
 *         interrupt the drawing process to stop animation.
 ***********************************************************************/
void LCD_Draw_Single_Color(uint16_t color) {
  /* Set full-screen drawing window */
  LCD_SetArea(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

  /* Loop through each pixel on the screen */
  for (uint16_t x = 0; x < LCD_WIDTH; x++) {
      for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
          LCD_WriteData(color);  /* Write pixel color to GRAM */

          /* Check for incoming UART command */
          if (rxDataReady) {
              EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);

              if (strncmp((const char *)rxBuffer, lcd_test_off, strlen(lcd_test_off)) == 0 ||
                  strncmp((const char *)rxBuffer, "LCD_BACK=", 9) == 0 ||
                  strncmp((const char *)rxBuffer, "DISP_VCC=OFF", 11) == 0) {
                  break;
              }

              /* Clear UART buffer and re-enable interrupts */
              rxDataReady = 0;
              memset(rxBuffer, 0, sizeof(rxBuffer));
              EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);
          }
      }
  }
}

/*********************************************************************************//**
 * @brief  Draw horizontal color bars across the entire LCD screen.
 *
 * This function divides the screen height into 8 horizontal bars,
 * each with a predefined color. The last bar adjusts its height to
 * fill any remaining pixels. Cursor-based drawing is used.
 *
 * @note   UART commands (LCD_TEST=OFF, LCD_BACK=, DISP_VCC=OFF) can
 *         interrupt the drawing process to stop animation.
 ******************************************************************************/
void LCD_Draw_ColorBars_Horizontal_Cursor(void) {
  /* Define color palette for horizontal bars */
  uint16_t colors[] = {
      COLOR_RED, COLOR_GREEN, COLOR_BLUE,
      COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
      COLOR_WHITE, COLOR_BLACK
  };

  uint8_t numBars   = sizeof(colors) / sizeof(colors[0]);
  uint16_t barHeight = LCD_HEIGHT / numBars;

  /* Loop through each color bar */
  for (uint8_t c = 0; c < numBars; c++) {
      uint16_t color  = colors[c];
      uint16_t height = (c == numBars - 1)
                                                  ? (LCD_HEIGHT - (barHeight * (numBars - 1)))  /* Adjust last bar */
                                                      : barHeight;

      /* Draw each row of the bar */
      for (uint16_t y = 0; y < height; y++) {
          for (uint16_t x = 0; x < LCD_WIDTH; x++) {
              LCD_SetCursor(x, (c * barHeight) + y);  /* Set GRAM cursor */
              LCD_WriteData(color);                   /* Draw pixel */

              /* Check for incoming UART command */
              if (rxDataReady) {
                  EUSART_IntDisable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);

                  if (strncmp((const char *)rxBuffer, lcd_test_off, strlen(lcd_test_off)) == 0 ||
                      strncmp((const char *)rxBuffer, "LCD_BACK=", 9) == 0 ||
                      strncmp((const char *)rxBuffer, "DISP_VCC=OFF", 11) == 0) {
                      break;
                  }

                  /* Clear UART buffer and re-enable interrupts */
                  rxDataReady = 0;
                  memset(rxBuffer, 0, sizeof(rxBuffer));
                  EUSART_IntEnable(CONSOLE_UART, EUSART_IEN_RXFL | EUSART_IEN_TXC);
              }
          }
      }
  }
}
/**
 * @brief  Draw a single 5x7 character on the LCD.
 *
 * @param  x      X-coordinate of the top-left pixel of the character
 * @param  y      Y-coordinate of the top-left pixel of the character
 * @param  ch     Character to draw (capital letters 'A'–'Z' only)
 * @param  color  16-bit color value for the character pixels
 *
 * @note   Only capital letters are supported. Invalid characters are ignored.
 */
void LCD_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color) {
  /* Validate character */
  if (ch < 'A' || ch > 'Z') return;

  /* Get pointer to 5x7 bitmap for the character */
  const uint8_t *bitmap = font5x7[ch - 'A'];

  /* Loop through each column of the character (5 columns) */
  for (int col = 0; col < 5; col++) {
      uint8_t line = bitmap[col];  /* Column bitmap */

      /* Loop through each row (7 rows) */
      for (int row = 0; row < 7; row++) {
          LCD_SetCursor(x + col, y + row);  /* Set pixel position */
          if (line & (1 << row)) {
              LCD_WriteData(color);         /* Draw pixel if bit is set */
          }
      }
  }
}

/**
 * @brief  Draw a string of capital letters on the LCD.
 *
 * @param  x      Starting X-coordinate of the first character
 * @param  y      Starting Y-coordinate of the first character
 * @param  str    Null-terminated string containing characters 'A'-'Z' and spaces
 * @param  color  16-bit color value for the characters
 *
 * @note   Each character is 5x7 pixels, with 1 pixel spacing between characters.
 *         Spaces advance the cursor by 6 pixels.
 */
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color) {
  while (*str) {
      if (*str >= 'A' && *str <= 'Z') {
          LCD_DrawChar(x, y, *str, color);  /* Draw character */
          x += 6;                            /* Move cursor (5px char + 1px space) */
      } else if (*str == ' ') {
          x += 6;                            /* Space between words */
      }
      str++;                                  /* Move to next character */
  }
}


/***************************************************************************//**
 * @brief  Initialize TIMER0 CC0 for PWM output on PD10
 ******************************************************************************/
void initTIMER(void)
{
  uint32_t timerFreq, topValue;                   // Variables for clock freq and top value
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;      // Default timer init structure
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT; // Default CC init structure
  timerInit.clkSel = 0;
  // Enable GPIO and TIMER0 clocks
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);

  // Configure PD10 as push-pull output, initial value = 0
  GPIO_PinModeSet(gpioPortD, 10, gpioModePushPull, 0);

  // Do not start counter immediately after initialization
  timerInit.enable = false;

  // Configure CC0 for PWM mode
  timerCCInit.mode = timerCCModePWM;              // Set compare mode to PWM
  timerCCInit.cmoa = timerOutputActionSet;        // Set output on compare match

  // Initialize TIMER0 with timerInit settings
  TIMER_Init(LCD_PWM_TIMER, &timerInit);

  // Route TIMER0 CC0 output to PD10
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;   // Enable CC0 output
  GPIO->TIMERROUTE[0].CC0ROUTE =
      (LCD_PWM_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)      // Select output port
      | (LCD_PWM_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);     // Select output pin

  // Initialize TIMER0 CC0 with CC config
  TIMER_InitCC(LCD_PWM_TIMER, 0, &timerCCInit);

  // Calculate top value for desired PWM frequency
  uint32_t presc = 1u << timerInit.prescale;   // prescale is an enum (2^n)
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / presc;
  topValue = timerFreq / PWM_FREQ;                 // Compute top value
  TIMER_TopSet(LCD_PWM_TIMER, topValue);           // Set top value

  // Start TIMER0
  TIMER_Enable(LCD_PWM_TIMER, true);

  // Enable CC0 interrupt (optional)
  TIMER_IntEnable(LCD_PWM_TIMER, TIMER_IEN_CC0);
}

/***************************************************************************//**
 * @brief  Set PWM duty cycle on TIMER0 CC0
 * @param  chan0 Duty cycle percentage (0–100)
 ******************************************************************************/
void PWMsetDutyCycle(uint8_t chan0)
{
  uint32_t top = TIMER_TopGet(LCD_PWM_TIMER);      // Get current top value

  // Set CC0 compare buffer value according to duty cycle %
  TIMER_CompareBufSet(LCD_PWM_TIMER, 0, (top * chan0) / 100);

  lcdDutyCycle = chan0;                            // Store current duty cycle
}



void LCD_Init(void) {
  LCD_GPIO_Init();               /* Initialize GPIO pins for LCD */
  LCD_SPI_Init();                /* Initialize SPI peripheral for LCD */
  LCD_RST();                     /* Hardware reset */
  SSD2119_PWR_ON_SEQUENCE();     /* Power-on sequence for SSD2119 controller */
  SSD2119_35U_SPI_INIT();        /* Send LCD initialization commands */
  initTIMER();
  PWMsetDutyCycle(lcdDutyCycle);
}
