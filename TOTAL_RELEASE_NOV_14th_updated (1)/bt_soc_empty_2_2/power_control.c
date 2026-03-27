/*
 * power_control.c
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#include "em_gpio.h"
#include "pin_config.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

// Device identifiers for switch-case in Change_Power
#define GPS_EN      11
#define GPS_RESET   7
#define SWOUT       9
#define MEM_PWR     10
#define ON          1
#define OFF         0

extern uint8_t gcmdbuff[];
void Change_Power(uint8_t device, uint8_t state);

/*********************************************************************//**
 * @brief Control power-related features based on device ID and state.
 *
 * @param device The target device (e.g., GPS_RESET, LCD, etc.)
 * @param state  ON or OFF (1 or 0)
 ***********************************************************************/

void Change_Power(uint8_t device, uint8_t state)
{
  GPIO_Port_TypeDef port;
  unsigned int pin;
  const char *name;

  switch (device) {
    case GPS_RESET:
      port = GPIO_GPS_RESET_PORT;
      pin  = GPIO_GPS_RESET_PIN;
      name = "GPS_RESET";
      break;

    case GPS_EN:
      port = GPIO_GPS_EN_PORT;
      pin  = GPIO_GPS_EN_PIN;
      name = "GPS_EN";
      break;

    case SWOUT:
      port = GPIO_SWOUT_PORT;
      pin  = GPIO_SWOUT_PIN;
      name = "SWOUT";
      break;

    case MEM_PWR:
      port = GPIO_MEM_PWR_PORT;
      pin  = GPIO_MEM_PWR_PIN;
      name = "MEM_PWR";
      break;

    case LCD_PWR:
      port = GPIO_DISP_VCC_PORT;
      pin  = GPIO_DISP_VCC_PIN;
      name = "DISP_VCC";
      break;

    case ANA_EN:
        port = ANALOG_ENABLE_PORT;
        pin  = ANALOG_ENABLE_PIN;
        name = "ANA_EN";
        break;

    default:
      return;
  }
  if (state == ON) {
      GPIO_PinOutSet(port, pin);
  } else {
      GPIO_PinOutClear(port, pin);
  }

  // Read back pin state from DOUT register
  int val = (GPIO->P[port].DOUT >> pin) & 1;

  // Print in requested format
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\n>> [VERIFY] 1. %s pin level now reads: %d\r\n",
           name, val);

  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
}



