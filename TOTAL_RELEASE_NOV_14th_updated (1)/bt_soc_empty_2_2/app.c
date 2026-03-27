/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "sl_common.h"
#include "sl_bt_api.h"
#include "app_assert.h"
#include "app.h"

void send_uart_data(const uint8_t *buff, size_t len);
static uint8_t advertising_set_handle = 0xff;
static bool ble_ready = false;
static bool ble_advertising = false;

// Application Init
SL_WEAK void app_init(void)
{
  // Custom init code
}

/**************************************************************************//**
 * @brief
 *    Application Periodic Processing Hook
 *
 * @details
 *    This weak function is called repeatedly in the main loop. Users can
 *    override this function to perform periodic actions, such as:
 *      - Polling sensors or ADCs
 *      - Updating displays
 *      - Processing user commands
 *
 * @note
 *    The `app_is_process_required()` function can be used to check if
 *    processing is needed before executing periodic tasks.
 *    Marked as SL_WEAK; define your own `app_process_action()` to override.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  if (app_is_process_required()) {
      // Your periodic tasks here
  }
}
/**************************************************************************//**
 * @brief
 *    BLE Stack Event Handler
 *
 * @details
 *    Handles incoming Bluetooth stack events:
 *      - System Boot: Creates advertising set and configures timing.
 *      - Connection Opened: Sends UART notification.
 *      - Connection Closed: Sends UART notification.
 *    Does not auto-start advertising after boot or disconnect; manual
 *    `ble_on_cmd()` must be called to start advertising.
 *
 * @param[in] evt
 *    Pointer to the Bluetooth stack event structure.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

    // ----------------------------
    // System Boot Event
    // ----------------------------
    case sl_bt_evt_system_boot_id:

      // Create advertising set
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising timing: min=160, max=160 (~100 ms)
      sc = sl_bt_advertiser_set_timing(advertising_set_handle, 160, 160, 0, 0);
      app_assert_status(sc);

      ble_ready = true; // Mark BLE stack ready
      // Do NOT auto-start advertising
      // ble_on_cmd();  <-- intentionally removed
      break;

      // ----------------------------
      // Connection Opened Event
      // ----------------------------
    case sl_bt_evt_connection_opened_id:
      send_uart_data((const uint8_t *)"BLE Connected\r\n", 15);
      break;

      // ----------------------------
      // Connection Closed Event
      // ----------------------------
    case sl_bt_evt_connection_closed_id:
      send_uart_data((const uint8_t *)"BLE Disconnected\r\n",
                     sizeof("BLE Disconnected\r\n") - 1);
      // Do NOT auto-restart advertising after disconnect
      // if (ble_ready) { ble_on_cmd(); }
      break;

    default:
      break;
  }
}


/**************************************************************************//**
 * @brief
 *    Enable BLE advertising
 *
 * @details
 *    Checks if the BLE stack is ready and not already advertising.
 *    Generates advertising data and starts connectable advertising.
 *    Updates internal state flag `ble_advertising` and sends
 *    debug messages to UART for all conditions and errors.
 *****************************************************************************/
void ble_on_cmd(void)
{
  sl_status_t sc;

  // --- 1. Check if BLE stack is initialized and ready ---
  if (!ble_ready) {
      send_uart_data((const uint8_t *)"BLE stack not ready\r\n", 21);
      return; // Cannot start advertising if BLE stack is not ready
  }

  // --- 2. Check if BLE is already ON ---
  if (ble_advertising) {
      send_uart_data((const uint8_t *)"BLE already ON\r\n", 16);
      return; // Nothing to do if already advertising
  }

  // --- 3. Generate advertising data (legacy advertiser) ---
  sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                             sl_bt_advertiser_general_discoverable);
  if (sc != SL_STATUS_OK) {
      send_uart_data((const uint8_t *)"BLE Advertiser Generate Data Failed\r\n", 37);
      return; // Cannot proceed if generating data failed
  }

  // --- 4. Start connectable advertising ---
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_legacy_advertiser_connectable);
  if (sc != SL_STATUS_OK) {
      send_uart_data((const uint8_t *)"BLE Advertiser Start Failed\r\n",
                     sizeof("BLE Advertiser Start Failed\r\n") - 1);
      return; // Exit if advertising start failed
  }

  // --- 5. Update system state and notify terminal ---
  ble_advertising = true; // Mark BLE as ON
  send_uart_data((const uint8_t *)"BLE Advertising ON\r\n",
                 sizeof("BLE Advertising ON\r\n") - 1);
}

/**************************************************************************//**
 * @brief
 *    Disable BLE advertising
 *
 * @details
 *    Checks if the BLE stack is ready and currently advertising.
 *    If advertising is active, stops the BLE advertiser and updates
 *    the internal state flag `ble_advertising`. Sends debug messages
 *    to UART for all conditions.
 *****************************************************************************/
void ble_off_cmd(void)
{
  // --- 1. Check if BLE stack is initialized and ready ---
  if (!ble_ready) {
      send_uart_data((const uint8_t *)"BLE stack not ready\r\n", 21);
      return; // Cannot stop advertising if BLE stack is not ready
  }

  // --- 2. Check if BLE is already OFF ---
  if (!ble_advertising) {
      send_uart_data((const uint8_t *)"BLE already OFF\r\n", 17);
      return; // Nothing to do if advertising is not active
  }

  // --- 3. Attempt to stop BLE advertising ---
  sl_status_t sc = sl_bt_advertiser_stop(advertising_set_handle);

  // --- 4. Check for errors in stopping the advertiser ---
  if (sc != SL_STATUS_OK) {
      send_uart_data((const uint8_t *)"BLE Advertiser Stop Failed\r\n",
                     sizeof("BLE Advertiser Stop Failed\r\n") - 1);
      return; // Exit if stopping failed
  }

  // --- 5. Update system state and notify terminal ---
  ble_advertising = false; // Mark BLE as OFF
  send_uart_data((const uint8_t *)"BLE Advertising OFF\r\n",
                 sizeof("BLE Advertising OFF\r\n") - 1);
}



