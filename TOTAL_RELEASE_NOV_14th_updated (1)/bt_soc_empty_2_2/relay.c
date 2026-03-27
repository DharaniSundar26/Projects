/*
 * relay.c
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#include"common.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ---- Timing (keep your existing values) ----
#ifndef RELAY_SWITCH_TIME
#define RELAY_SWITCH_TIME   6   // ms (pulse width)
#endif
#ifndef RELAY_SETTLE_TIME
#define RELAY_SETTLE_TIME   100   // ms (post-pulse settle)
#endif

// ---- Active level (flip for active-low drivers) ----
#define RELAY_PORT    gpioPortC
#define RELAY_ON_LEVEL   1
#define RELAY_OFF_LEVEL  0
#define RLY_KCOM      0
#define RLY_KCH1      1
#define RLY_KCH2      2
#define RLY_KCH3      3
#define RLY_KCH4      4
#define RLY_KCH4S     5
#define RLY_KCRNG1    6
#define RLY_KCRNG2    7
#define RLY_KCRNG4    8
#define RLY_KCAL      9
#define RELAY_CH_MAX  10


// -------------------- Command handler (GPIO version) -----------------
uint8_t adc_config_flag = false;

extern void send_uart_data(const uint8_t *buff, size_t len);
extern void zdelay(uint32_t d);
extern uint8_t gcmdbuff[];
extern void sl_sleeptimer_delay_millisecond(uint32_t ms);

/**
 * @brief Turn OFF Relay Channel 1 (KCH1).
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCH1 relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 */
void sr_ch1_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   /**< Prevent unused-parameter compiler warning */
  (void)len;     /**< Prevent unused-parameter compiler warning */
  uint8_t index; /**< Local loop variable for GPIO clearing */

  // Step 1: Disable latch outputs by pulling PD7 low
  GPIO_PinOutClear(gpioPortD , 7);

  // Step 2: Drive relay CH1 pin high (trigger OFF action)
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH1);

  // Step 3: Wait for relay contacts to settle
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Reset all relay GPIO pins to low state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 5: Re-enable latch outputs by setting PD7 high
  GPIO_PinOutSet(gpioPortD , 7);

  // Step 6: Format and send acknowledgment message via UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH1 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Short software delay to prevent immediate re-trigger
  zdelay(0xFFFF);
}

/*********************************************************************//**
 * @brief Turn OFF Relay Channel 2 (KCH2).
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCH2 relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 **********************************************************************/
void sr_ch2_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   /**< Prevent unused-parameter compiler warning */
  (void)len;     /**< Prevent unused-parameter compiler warning */
  uint8_t index; /**< Local loop variable for GPIO clearing */

  // Step 1: Disable latch outputs by pulling PD7 low
  GPIO_PinOutClear(gpioPortD , 7);

  // Step 2: Drive relay CH2 pin high (trigger OFF action)
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH2);

  // Step 3: Wait for relay contacts to settle
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Reset all relay GPIO pins to low state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 5: Re-enable latch outputs by setting PD7 high
  GPIO_PinOutSet(gpioPortD , 7);

  // Step 6: Format and send acknowledgment message via UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH2 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Short software delay to prevent immediate re-trigger
  zdelay(0xFFFF);
}
/*********************************************************************//**
 * @brief Turn OFF Relay Channel 3 (KCH3).
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCH3 relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 ************************************************************************/
void sr_ch3_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   /**< Prevent unused-parameter compiler warning */
  (void)len;     /**< Prevent unused-parameter compiler warning */
  uint8_t index; /**< Local loop variable for GPIO clearing */

  // Step 1: Disable latch outputs by pulling PD7 low
  GPIO_PinOutClear(gpioPortD , 7);

  // Step 2: Drive relay CH3 pin high (trigger OFF action)
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH3);

  // Step 3: Wait for relay contacts to settle
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Reset all relay GPIO pins to low state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 5: Re-enable latch outputs by setting PD7 high
  GPIO_PinOutSet(gpioPortD , 7);

  // Step 6: Format and send acknowledgment message via UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH3 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Short software delay to prevent immediate re-trigger
  zdelay(0xFFFF);
}
/*********************************************************************//**
 * @brief Turn OFF Relay Channel 4 (KCH4).
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCH4 relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 ************************************************************************/
void sr_ch4_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;
  (void)len;
  uint8_t index;
  // Clear the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutClear(gpioPortD , 7);
  // High Relay CHL2 pin
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH4);
  // Delay for Settle the relay
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);
  // All Relay low
  for(index = 0;index < RELAY_CH_MAX ; index ++)
    GPIO_PinOutClear(gpioPortC,index);

  // Set the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutSet(gpioPortD , 7);
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH4 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}
/*********************************************************************//**
 * @brief Turn OFF Relay Channel 4S (KCH4S).
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCH4s relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 ************************************************************************/
void sr_ch4S_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;
  (void)len;
  uint8_t index;
  // Clear the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutClear(gpioPortD , 7);
  // High Relay CHL2 pin
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH4S);
  // Delay for Settle the relay
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);
  // All Relay low
  for(index = 0;index < RELAY_CH_MAX ; index ++)
    GPIO_PinOutClear(gpioPortC,index);

  // Set the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutSet(gpioPortD , 7);
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH4S RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));
  zdelay(0xFFFF);
}

/****************************************************************************//**
 * @brief Turn OFF Relay KCRNG1.
 *
 * This function performs the following sequence:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the KCRNG1 relay pin high to generate the OFF pulse.
 *  3. Waits for the relay contacts to mechanically settle.
 *  4. Clears all relay GPIO outputs to prevent unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART for confirmation.
 *  7. Applies a short software delay to avoid immediate re-trigger.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 ****************************************************************************/
void sr_kcrng1_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   /**< Suppress unused-parameter compiler warning */
  (void)len;     /**< Suppress unused-parameter compiler warning */
  uint8_t index; /**< Local variable for GPIO clearing loop */

  // Step 1: Disable latch outputs by pulling PD7 low
  GPIO_PinOutClear(gpioPortD , 7);

  // Step 2: Drive KCRNG1 relay pin high (trigger OFF action)
  GPIO_PinOutSet(RELAY_PORT , RLY_KCRNG1);

  // Step 3: Wait for relay to settle mechanically
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Clear all relay control GPIO pins
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 5: Re-enable latch outputs by setting PD7 high
  GPIO_PinOutSet(gpioPortD , 7);

  // Step 6: Format and send acknowledgment message via UART
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG1 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Short software delay before next command
  zdelay(0xFFFF);
}
// ===================== KCRNG2 RELAY OFF ========================= //

/***************************************************************************//**
 * @brief
 *    Turn OFF a specific KCRNG relay channel (CH2)
 *
 * @details
 *    Disables the output register, momentarily activates the relay channel
 *    to switch it OFF, clears all relay channels, re-enables the register,
 *    and sends an ACK over UART confirming the relay OFF state.
 *
 * @param[in] cbuff
 *    Pointer to command buffer (unused in this function)
 *
 * @param[in] len
 *    Length of command buffer (unused)
 *
 * @note
 *    - REGISTER_OUT_DIS (PD7) is toggled to protect outputs during relay switching
 *    - Blocking delays are used to allow relay contacts to settle
 ******************************************************************************/
void sr_kcrng2_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff; // Unused parameter
  (void)len;   // Unused parameter

  uint8_t index; // Loop index for clearing all relays

  // Disable relay register output to protect relays during switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Activate Relay CH2 (set HIGH) to switch it OFF
  GPIO_PinOutSet(RELAY_PORT, RLY_KCRNG2);

  // Delay to allow relay contacts to settle
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay channels (set LOW)
  for (index = 0; index < RELAY_CH_MAX; index++) {
      GPIO_PinOutClear(gpioPortC, index);
  }

  // Re-enable relay register output
  GPIO_PinOutSet(gpioPortD, 7);

  // Send ACK message via UART confirming relay OFF
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG2 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Optional blocking delay to ensure UART transmission completes
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn OFF the KCRNG4 relay channel.
 *
 * @details
 *   This function disables the REGISTER_OUT_DIS pin (PD7), sets the
 *   KCRNG4 relay pin HIGH momentarily, allows relay contacts to settle,
 *   clears all relay outputs, and then re-enables REGISTER_OUT_DIS.
 *   Finally, it sends an ACK message over UART indicating that the
 *   KCRNG4 relay has been turned OFF.
 *
 * @param[in] cbuff
 *   Unused input command buffer (kept for compatibility).
 *
 * @param[in] len
 *   Unused length of command buffer (kept for compatibility).
 *
 * @note
 *   - REGISTER_OUT_DIS (PD7) protects outputs during switching.
 *   - Uses blocking delay (`sl_sleeptimer_delay_millisecond`) for relay
 *     settle time.
 *   - Ends with another blocking delay (`zdelay`) after ACK transmission.
 ******************************************************************************/
void sr_kcrng4_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning.
  (void)len;     ///< Suppress unused parameter warning.

  uint8_t index; ///< Loop counter for clearing all relay channels.

  // Step 1: Disable the REGISTER_OUT_DIS pin (PD7) before switching relays.
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set the KCRNG4 relay pin HIGH to activate it.
  GPIO_PinOutSet(RELAY_PORT, RLY_KCRNG4);

  // Step 3: Wait for relay contacts to settle (to avoid chattering).
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
    {
      GPIO_PinOutClear(gpioPortC, index);
    }

  // Step 5: Re-enable the REGISTER_OUT_DIS pin (PD7) after switching.
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 6: Send ACK message over UART confirming KCRNG4 relay OFF.
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG4 RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Apply additional blocking delay (hardware-specific).
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn OFF the KCAL relay channel.
 *
 * @details
 *   This function safely switches OFF the KCAL relay. It temporarily disables
 *   the REGISTER_OUT_DIS pin (PD7), activates the KCAL relay output pin, waits
 *   for the relay contacts to settle, clears all relay outputs on Port C,
 *   and finally re-enables REGISTER_OUT_DIS. After completing the switching,
 *   it sends an ACK message via UART to confirm the action.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API consistency).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 *
 * @note
 *   - REGISTER_OUT_DIS (PD7) prevents glitches during relay switching.
 *   - A blocking delay (`sl_sleeptimer_delay_millisecond`) ensures relay
 *     settling before clearing outputs.
 *   - Ends with an additional blocking delay (`zdelay`).
 ******************************************************************************/
void sr_kcal_relay_off(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning.
  (void)len;     ///< Suppress unused parameter warning.

  uint8_t index; ///< Loop counter for clearing relay channels.

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching relays.
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set the KCAL relay control pin HIGH.
  GPIO_PinOutSet(RELAY_PORT, RLY_KCAL);

  // Step 3: Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Step 4: Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
    {
      GPIO_PinOutClear(gpioPortC, index);
    }

  // Step 5: Re-enable REGISTER_OUT_DIS pin (PD7) after switching.
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 6: Send ACK message via UART confirming KCAL relay OFF.
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCAL RELAY:OFF\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 7: Apply extra blocking delay (hardware-specific).
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCH1.
 *
 * @details
 *   This function enables the KCH1 relay by first disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all other relay control pins HIGH
 *   except KCH1, and then activating the KCOM pin to connect the relay
 *   common line. After waiting for the relay switching time, all relay
 *   outputs are cleared and REGISTER_OUT_DIS is re-enabled. An ACK message
 *   is sent over UART to confirm the relay ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused command buffer length (kept for API compatibility).
 *
 * @note
 *   - REGISTER_OUT_DIS (PD7) is used to avoid glitches during switching.
 *   - KCOM pin is required to complete the relay circuit.
 *   - Uses `RELAY_SWITCH_TIME` for switching delay.
 ******************************************************************************/
void sr_ch1_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning.
  (void)len;     ///< Suppress unused parameter warning.

  uint8_t index; ///< Loop counter for GPIO relay control.

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching.
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay channel GPIOs HIGH except the current one (KCH1).
  for (index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if (index != RLY_KCH1)
        {
          GPIO_PinOutSet(gpioPortC, index);
        }
    }

  // Step 3: Activate the KCOM pin to connect the relay common line.
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay to settle in ON position.
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay channel outputs to finalize ON state.
  for (index = 0; index < RELAY_CH_MAX; index++)
    {
      GPIO_PinOutClear(gpioPortC, index);
    }

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching.
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCH1 relay ON.
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH1 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply additional blocking delay (hardware-specific).
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCH2.
 *
 * @details
 *   This function enables the KCH2 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCH2 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 *
 * @note
 *   - REGISTER_OUT_DIS (PD7) prevents glitches while switching relays.
 *   - `RELAY_SWITCH_TIME` defines the blocking delay for relay activation.
 ******************************************************************************/
void sr_ch2_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning.
  (void)len;     ///< Suppress unused parameter warning.

  uint8_t index; ///< Loop counter for relay GPIO handling.

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching.
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCH2).
  for (index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if (index != RLY_KCH2)
        {
          GPIO_PinOutSet(gpioPortC, index);
        }
    }

  // Step 3: Activate the KCOM pin to connect the relay common line.
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state.
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state.
  for (index = 0; index < RELAY_CH_MAX; index++)
    {
      GPIO_PinOutClear(gpioPortC, index);
    }

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching.
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCH2 relay ON.
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH2 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific).
  zdelay(0xFFFF);
}
/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCH3.
 *
 * @details
 *   This function enables the KCH3 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCH3 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 *
 * @note
 *   - REGISTER_OUT_DIS (PD7) prevents glitches while switching relays.
 *   - `RELAY_SWITCH_TIME` defines the blocking delay for relay activation.
 ******************************************************************************/
void sr_ch3_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning.
  (void)len;     ///< Suppress unused parameter warning.

  uint8_t index; ///< Loop counter for relay GPIO handling.

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching.
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCH3).
  for (index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if (index != RLY_KCH3)
        {
          GPIO_PinOutSet(gpioPortC, index);
        }
    }

  // Step 3: Activate the KCOM pin to connect the relay common line.
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state.
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state.
  for (index = 0; index < RELAY_CH_MAX; index++)
    {
      GPIO_PinOutClear(gpioPortC, index);
    }

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching.
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCH3 relay ON.
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH3 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific).
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCH4.
 *
 * @details
 *   This function enables the KCH4 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCH4 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_ch4_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCH4)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCH4)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCH4 relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH4 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCH4S.
 *
 * @details
 *   This function enables the KCH4S relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCH4S pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_ch4S_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCH4S)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCH4S)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCH4S relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCH4S RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}
/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCRNG1.
 *
 * @details
 *   This function enables the KCRNG1 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCRNG1 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_kcrng1_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCRNG1)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCRNG1)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCRNG1 relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG1 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCRNG2.
 *
 * @details
 *   This function enables the KCRNG2 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCRNG2 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_kcrng2_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCRNG2)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCRNG2)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCRNG2 relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG2 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCRNG4.
 *
 * @details
 *   This function enables the KCRNG4 relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCRNG4 pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_kcrng4_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCRNG4)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCRNG4)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCRNG4 relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCRNG4 RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}

/***************************************************************************//**
 * @brief
 *   Turn ON relay channel KCAL.
 *
 * @details
 *   This function enables the KCAL relay by temporarily disabling the
 *   REGISTER_OUT_DIS pin (PD7), setting all relay GPIO pins HIGH except
 *   for the KCAL pin, and then enabling the KCOM pin to connect the
 *   common line. After waiting for the relay to switch, it clears all
 *   relay outputs and re-enables REGISTER_OUT_DIS. Finally, it sends
 *   an ACK message via UART confirming the ON state.
 *
 * @param[in] cbuff
 *   Unused command buffer (kept for API compatibility).
 *
 * @param[in] len
 *   Unused length of the command buffer.
 ******************************************************************************/
void sr_kcal_relay_on(uint8_t *cbuff, uint16_t len)
{
  (void)cbuff;   ///< Suppress unused parameter warning
  (void)len;     ///< Suppress unused parameter warning

  uint8_t index; ///< Loop counter for relay GPIO handling

  // Step 1: Disable REGISTER_OUT_DIS pin (PD7) before switching
  GPIO_PinOutClear(gpioPortD, 7);

  // Step 2: Set all relay GPIOs HIGH except the current one (KCAL)
  for(index = 0; index < RELAY_CH_MAX - 1; index++)
    {
      if(index != RLY_KCAL)
        GPIO_PinOutSet(gpioPortC, index);
    }

  // Step 3: Activate the KCOM pin to connect the relay common line
  GPIO_PinOutSet(RELAY_PORT, RLY_KCOM);

  // Step 4: Wait for relay contacts to settle in ON state
  sl_sleeptimer_delay_millisecond(RELAY_SWITCH_TIME);

  // Step 5: Clear all relay GPIO outputs to finalize ON state
  for(index = 0; index < RELAY_CH_MAX; index++)
    GPIO_PinOutClear(gpioPortC, index);

  // Step 6: Re-enable REGISTER_OUT_DIS pin (PD7) after switching
  GPIO_PinOutSet(gpioPortD, 7);

  // Step 7: Send ACK message via UART confirming KCAL relay ON
  snprintf((char *)gcmdbuff, sizeof(gcmdbuff),
           "\r\r\nACK:KCAL RELAY:ON\r\n");
  send_uart_data(gcmdbuff, (uint16_t)strlen((const char *)gcmdbuff));

  // Step 8: Apply extra blocking delay (hardware-specific)
  zdelay(0xFFFF);
}

/*********************************************************************//**
 * @brief Turn OFF Relay All Channel
 *
 * This function performs the following steps:
 *  1. Disables the output register latch (PD7 low).
 *  2. Drives the all relay pin high to trigger the OFF pulse.
 *  3. Waits for the relay to mechanically settle.
 *  4. Clears all relay GPIOs to ensure no unintended activation.
 *  5. Re-enables the output register latch (PD7 high).
 *  6. Sends an acknowledgment message over UART.
 *  7. Inserts a short software delay before allowing new commands.
 *
 * @param[in] cbuff Command buffer pointer (unused).
 * @param[in] len   Length of command buffer (unused).
 ************************************************************************/
void All_relay_off()
{
  uint8_t index;
  // Keep your ADS124 config preamble
  // Clear the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutClear(gpioPortD , 7);
  // High Relay all pins
  GPIO_PinOutSet(RELAY_PORT , RLY_KCH1);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT , RLY_KCH2);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT , RLY_KCH3);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT , RLY_KCH4);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT , RLY_KCH4S);
  //  Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT , RLY_KCRNG1);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT, RLY_KCRNG2);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT, RLY_KCRNG4);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }

  GPIO_PinOutSet(RELAY_PORT, RLY_KCAL);
  // Wait for relay contacts to settle.
  sl_sleeptimer_delay_millisecond(RELAY_SETTLE_TIME);

  // Clear all relay outputs (Port C) to ensure OFF state.
  for (index = 0; index < RELAY_CH_MAX; index++)
  {
    GPIO_PinOutClear(gpioPortC, index);
  }
  // Set the REGISTER_OUT_DIS pin PD7
  GPIO_PinOutSet(gpioPortD , 7);
}
