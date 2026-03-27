/*
 * Flash_Test.c
 *
 *  Created on: 08-July-2025
 *      Author: Bavatharani
 */
#include "em_device.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_eusart.h"
#include "em_usart.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

#define MAX_FLASH_READ_SIZE 256

/***************************************************************************//**
 * @brief
 *    Assert the SPI flash chip select (CS) line.
 *
 * @details
 *    Pulls the CS pin low to select the flash chip for SPI communication
 *    before sending or receiving SPI data.
 ******************************************************************************/
static inline void Flash_Select(void)
{
  GPIO_PinOutClear(FLASH_CS_PORT, FLASH_CS_PIN);
}

/***************************************************************************//**
 * @brief
 *    Deassert the SPI flash chip select (CS) line.
 *
 * @details
 *    Pulls the CS pin high to deselect the flash chip, ending SPI communication.
 ******************************************************************************/
static inline void Flash_Deselect(void)
{
  GPIO_PinOutSet(FLASH_CS_PORT, FLASH_CS_PIN);
}

/***************************************************************************//**
 * @brief
 *    Power on the SPI flash memory.
 *
 * @details
 *    Configures the GPIO pin controlling flash power as a push-pull output,
 *    sets the pin high to supply voltage to the flash memory, and waits briefly
 *    to ensure stable operation before any SPI communication.
 ******************************************************************************/
void flash_power_on(void)
{
  // Configure flash power pin as push-pull output, initial value low
  GPIO_PinModeSet(GPIO_MEM_PWR_PORT, GPIO_MEM_PWR_PIN, gpioModePushPull, 0);

  // Set flash power pin high to turn on the flash memory
  GPIO_PinOutSet(GPIO_MEM_PWR_PORT, GPIO_MEM_PWR_PIN);

  // Delay briefly to allow flash to stabilize
  zdelay(0x1000);
}

/***************************************************************************//**
 * @brief
 *    Ensure flash memory is powered before access.
 *
 * @details
 *    Checks if the flash power GPIO pin is low (flash is off). If so, it sets
 *    the pin high and waits briefly to allow the flash to stabilize before
 *    any SPI operations.
 ******************************************************************************/
void flash_access_prepare(void)
{
  // Check if flash power pin is low (flash off)
  if (!GPIO_PinOutGet(GPIO_MEM_PWR_PORT, GPIO_MEM_PWR_PIN)) {
      // Set flash power pin high to enable flash
      GPIO_PinOutSet(GPIO_MEM_PWR_PORT, GPIO_MEM_PWR_PIN);

      // Short delay to ensure flash power is stable
      zdelay(0x1000);
  }
}

/***************************************************************************//**
 * @brief
 *    Transmit and receive a single byte over SPI using USART1.
 *
 * @param[in] data
 *    The byte to transmit.
 *
 * @return
 *    The byte received from the SPI slave during transmission.
 *
 * @details
 *    Waits for the transmit buffer to be empty, sends a byte, waits for data
 *    to be received, then reads and returns the received byte.
 ******************************************************************************/
static uint8_t SPI_TxRx(uint8_t data)
{
  // Wait until transmit buffer is empty
  while (!(USART1->STATUS & USART_STATUS_TXBL));

  // Transmit the byte over SPI
  USART_Tx(USART1, data);

  // Wait until a byte is received
  while (!(USART1->STATUS & USART_STATUS_RXDATAV));

  // Return the received byte
  return (uint8_t)USART_Rx(USART1);
}

/***************************************************************************//**
 * @brief
 *    Wait until the flash memory is no longer busy.
 *
 * @details
 *    Continuously polls the Status Register 1 of the flash memory and checks
 *    the Busy bit (bit 0). The function returns only when the flash is ready
 *    for the next command.
 ******************************************************************************/
void Flash_WaitBusy(void) {
  uint8_t status;
  uint32_t guard = 0; // finite guard to avoid deadlock

  do {
      Flash_Select();
      SPI_TxRx(CMD_READ_STATUS1); // SR1
      status = SPI_TxRx(0xFF);
      Flash_Deselect();

      if (++guard > 100000) {           // ~safe upper bound (tune per part)
          send_uart_data((const uint8_t *)"ERR:FLASH_BUSY_TIMEOUT\r\n",
                         sizeof("ERR:FLASH_BUSY_TIMEOUT\r\n") - 1);
          break;
      }


  } while (status & 0x01); // WIP=1 => busy
}

/***************************************************************************//**
 * @brief
 *    Enable write operations on the flash memory.
 *
 * @details
 *    Sends the Write Enable (WREN) command to the flash device, allowing
 *    subsequent write or erase operations.
 ******************************************************************************/
void Flash_WriteEnable(void) {
  Flash_Select();
  SPI_TxRx(CMD_WRITE_ENABLE);
  Flash_Deselect();

  /* Verify WEL (SR1 bit1) is set */
  uint8_t  sr1 = 0;
  uint32_t tries = 0;
  do {
      Flash_Select();
      SPI_TxRx(CMD_READ_STATUS1);
      sr1 = SPI_TxRx(0xFF);
      Flash_Deselect();
  } while (!(sr1 & 0x02) && (++tries < 1000));

}

/***************************************************************************//**
 * @brief
 *    Write a single byte to flash memory (page program).
 *
 * @param[in] addr
 *    24-bit flash memory address to write the byte.
 * @param[in] data
 *    The byte value to write.
 *
 * @details
 *    This function enables writing, sends the Page Program command with
 *    the address, writes the byte, deselects the flash, and waits until
 *    the flash completes the operation.
 ******************************************************************************/
void Flash_PageProgram(uint32_t addr, uint8_t data) {
  Flash_WriteEnable();                   // Enable write operations
  Flash_Select();                        // Select flash (CS low)
  SPI_TxRx(CMD_PAGE_PROGRAM);            // Send Page Program command
  SPI_TxRx((addr >> 16) & 0xFF);         // Send high byte of address
  SPI_TxRx((addr >> 8) & 0xFF);          // Send middle byte of address
  SPI_TxRx(addr & 0xFF);                 // Send low byte of address
  SPI_TxRx(data);                         // Send the data byte
  Flash_Deselect();                       // Deselect flash (CS high)
  Flash_WaitBusy();                       // Wait for write to complete
}

/***************************************************************************//**
 * @brief
 *    Read a single byte from flash memory.
 *
 * @param[in] addr
 *    24-bit flash memory address to read from.
 *
 * @return
 *    The byte value read from flash.
 *
 * @details
 *    This function selects the flash, sends the Read Data command with the
 *    address, reads one byte, and then deselects the flash.
 ******************************************************************************/
uint8_t Flash_ReadByte(uint32_t addr) {
  uint8_t val;

  Flash_Select();                        // Select flash (CS low)
  SPI_TxRx(CMD_READ_DATA);               // Send Read Data command
  SPI_TxRx((addr >> 16) & 0xFF);         // Send high byte of address
  SPI_TxRx((addr >> 8) & 0xFF);          // Send middle byte of address
  SPI_TxRx(addr & 0xFF);                 // Send low byte of address
  val = SPI_TxRx(0xFF);                  // Read byte from flash
  Flash_Deselect();                       // Deselect flash (CS high)

  return val;                             // Return read byte
}

/***************************************************************************//**
 * @brief
 *    Erase a 4KB sector of flash memory.
 *
 * @param[in] addr
 *    24-bit flash memory address within the sector to erase.
 *
 * @details
 *    This function enables write operations, sends the Sector Erase command
 *    with the target address, deselects the flash, and waits until the erase
 *    operation completes.
 ******************************************************************************/
void flash_erase_sector(uint32_t addr) {
  Flash_WriteEnable();                   // Enable write operations
  Flash_Select();                        // Select flash (CS low)
  SPI_TxRx(CMD_SECTOR_ERASE);            // Send Sector Erase command
  SPI_TxRx((addr >> 16) & 0xFF);         // Send high byte of address
  SPI_TxRx((addr >> 8) & 0xFF);          // Send middle byte of address
  SPI_TxRx(addr & 0xFF);                 // Send low byte of address
  Flash_Deselect();                       // Deselect flash (CS high)
  Flash_WaitBusy();                       // Wait for erase to complete
}
/***************************************************************************//**
 * @brief
 *    Write multiple bytes (up to 256) to flash memory.
 *
 * @param[in] addr
 *    24-bit flash memory start address for writing.
 * @param[in] buf
 *    Pointer to the data buffer to write.
 * @param[in] len
 *    Number of bytes to write (max 256 bytes per page).
 *
 * @details
 *    This function enables write operations, selects the flash,
 *    sends the Page Program command and 24-bit address, writes the data
 *    bytes sequentially, deselects the flash, and waits until the write
 *    operation completes.
 ******************************************************************************/
void flash_write_data(uint32_t addr, uint8_t *buf, uint16_t len)
{
  Flash_WriteEnable();                   // Enable flash write
  Flash_Select();                        // Select flash (CS low)
  SPI_TxRx(CMD_PAGE_PROGRAM);            // Send Page Program command
  SPI_TxRx((addr >> 16) & 0xFF);         // Send high byte of address
  SPI_TxRx((addr >> 8) & 0xFF);          // Send middle byte of address
  SPI_TxRx(addr & 0xFF);                 // Send low byte of address

  // Send data bytes sequentially
  for (uint16_t i = 0; i < len; i++) {
      SPI_TxRx(buf[i]);                  // Write each byte
  }

  Flash_Deselect();                       // Deselect flash (CS high)
  Flash_WaitBusy();                       // Wait for write completion
}

/***************************************************************************//**
 * @brief
 *    Read multiple bytes from flash memory.
 *
 * @param[in] addr
 *    24-bit flash memory start address to read from.
 * @param[out] buf
 *    Pointer to the buffer to store the read data.
 * @param[in] len
 *    Number of bytes to read.
 *
 * @details
 *    This function reads sequential bytes from flash memory by calling
 *    Flash_ReadByte() in a loop for the requested length.
 ******************************************************************************/
void Flash_ReadData(uint32_t addr, uint8_t *buf, uint32_t len)
{
  if (len == 0U || buf == NULL) {
      return;
  }
  // Loop over each byte to read
  for(uint32_t i = 0; i < len; i++) {
      buf[i] = Flash_ReadByte(addr + i);  // Read one byte at a time
  }
}

/***************************************************************************//**
 * @brief
 *    Initialize SPI interface for external flash memory.
 *
 * @details
 *    This function enables the USART1 clock, configures the GPIO pins for
 *    SPI (CS, MOSI, MISO, SCK), sets the USART routing registers, and initializes
 *    the USART in synchronous master mode with 1 MHz baudrate, MSB first, and
 *    SPI mode 0 (clock idle low, sample on rising edge).
 ******************************************************************************/
void init_spi_flash(void) {
  CMU_ClockEnable(cmuClock_USART1, true); // Enable USART1 clock

  // Configure flash SPI pins
  GPIO_PinModeSet(FLASH_CS_PORT, FLASH_CS_PIN, gpioModePushPull, 1);   // CS idle HIGH
  GPIO_PinModeSet(FLASH_MOSI_PORT, FLASH_MOSI_PIN, gpioModePushPull, 0); // MOSI output
  GPIO_PinModeSet(FLASH_MISO_PORT, FLASH_MISO_PIN, gpioModeInput, 0);   // MISO input
  GPIO_PinModeSet(FLASH_SCK_PORT, FLASH_SCK_PIN, gpioModePushPull, 0);   // SCK output

  CMU_ClockEnable(cmuClock_USART1, true); // Ensure USART1 clock is enabled

  // Configure USART1 routing to appropriate GPIO pins
  GPIO->USARTROUTE[1].TXROUTE  = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) | (FLASH_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[1].RXROUTE  = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) | (FLASH_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[1].CLKROUTE = (gpioPortA << _GPIO_USART_CLKROUTE_PORT_SHIFT) | (FLASH_SCK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[1].ROUTEEN  = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_CLKPEN;

  // Initialize USART1 in synchronous SPI master mode
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;
  init.baudrate  = 1000000;   // 1 MHz SPI clock
  init.msbf      = true;      // MSB first
  init.master    = true;      // Master mode
  init.clockMode = usartClockMode0; // SPI mode 0
  USART_InitSync(USART1, &init);    // Apply configuration
}

/***************************************************************************//**
 * @brief
 *    Read and print the JEDEC ID of the SPI flash memory.
 *
 * @details
 *    Sends the standard JEDEC ID command (0x9F) to the flash chip, reads
 *    manufacturer ID, memory type, and capacity, and prints the result
 *    via UART.
 ******************************************************************************/
void Flash_ReadJedecID(void) {
  uint8_t id[3];                   // Array to hold JEDEC ID bytes

  Flash_Select();                   // Select flash (CS low)
  SPI_TxRx(0x9F);                   // Send JEDEC ID command
  id[0] = SPI_TxRx(0xFF);           // Read Manufacturer ID
  id[1] = SPI_TxRx(0xFF);           // Read Memory Type
  id[2] = SPI_TxRx(0xFF);           // Read Capacity
  Flash_Deselect();                 // Deselect flash (CS high)

  char msg[64];
  snprintf(msg, sizeof(msg), "Flash JEDEC ID: %02X %02X %02X\r\n", id[0], id[1], id[2]);
  send_uart_data((uint8_t*)msg, strlen(msg)); // Send JEDEC ID via UART
}
