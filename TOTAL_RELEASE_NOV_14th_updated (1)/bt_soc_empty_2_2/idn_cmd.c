/*
 * idn_cmd.c
 *
 *  Created on: 02-Apr-2022
 *      Author: zumi
 */

#include "common.h"

/** @brief IDN Command processing
 *
 *  This function processes the IDN command
 *
 *  @param  cbuff[IN] : Command Received from the UART terminal
 *          len[IN]   : Length of the received command string
 *
 *  @return none
 *
 */
void idn_cmd(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;
  (void)len;
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff), "\r\nACK:*IDN?=%s\r\n", FW_VERSION);
  send_uart_data(gcmdbuff, strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}

