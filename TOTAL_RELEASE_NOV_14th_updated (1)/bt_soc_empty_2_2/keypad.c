#include "common.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_chip.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define MAX_ROWS 4
#define MAX_COLS 4

// Rows
#define ROW_PORT_0 gpioPortB
#define ROW_PIN_0  0
#define ROW_PORT_1 gpioPortB
#define ROW_PIN_1  1
#define ROW_PORT_2 gpioPortB
#define ROW_PIN_2  2
#define ROW_PORT_3 gpioPortB
#define ROW_PIN_3  3

// Columns
#define COL_PORT_0 gpioPortB
#define COL_PIN_0  4
#define COL_PORT_1 gpioPortB
#define COL_PIN_1  5
#define COL_PORT_2 gpioPortB
#define COL_PIN_2  6
#define COL_PORT_3 gpioPortB
#define COL_PIN_3  7
#define DEBOUNCE_DELAY_MS 10

// Keypad map
static const char key_map[4][4] = {
    {'A','3','2','1'},
    {'D','#','0','*'},
    {'B','6','5','4'},
    {'C','9','8','7'}
};

static char last_key = '\0';
static uint8_t g_lastRow = 0;
static uint8_t g_lastCol = 0;
extern void send_uart_data(const uint8_t *buff, size_t len);
/**************************************************************************//**
 * @brief
 *    Simple blocking millisecond delay
 *
 * @details
 *    Implements a crude software delay using nested loops.
 *    Each iteration approximates ~1 ms delay depending on system clock.
 *    Uses a volatile counter to prevent compiler optimization.
 *
 * @param[in] ms
 *    Number of milliseconds to delay
 *
 * @note
 *    This is a busy-wait loop. The actual delay depends on CPU clock
 *    and may need calibration for accurate timing.
 *****************************************************************************/
static void zdelay_ms(uint32_t ms)
{
  volatile uint32_t count;

  for (uint32_t i = 0; i < ms; i++) {
      count = 38000;  // Approximate number of NOPs for 1 ms
      while (count--) {
          __NOP();
      }
  }
}

/**************************************************************************//**
 * @brief
 *    Initialize 4x4 Keypad GPIO
 *
 * @details
 *    - Enables the GPIO clock.
 *    - Configures row pins as push-pull outputs and sets them HIGH.
 *    - Configures column pins as inputs with pull-up resistors.
 *    - Prepares keypad for row-column scanning.
 *****************************************************************************/
void keypad_init(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure rows as outputs (push-pull, default HIGH)
  GPIO_PinModeSet(ROW_PORT_0, ROW_PIN_0, gpioModePushPull, 1);
  GPIO_PinModeSet(ROW_PORT_1, ROW_PIN_1, gpioModePushPull, 1);
  GPIO_PinModeSet(ROW_PORT_2, ROW_PIN_2, gpioModePushPull, 1);
  GPIO_PinModeSet(ROW_PORT_3, ROW_PIN_3, gpioModePushPull, 1);

  // Configure columns as inputs with pull-up
  GPIO_PinModeSet(COL_PORT_0, COL_PIN_0, gpioModeInputPull, 1);
  GPIO_PinModeSet(COL_PORT_1, COL_PIN_1, gpioModeInputPull, 1);
  GPIO_PinModeSet(COL_PORT_2, COL_PIN_2, gpioModeInputPull, 1);
  GPIO_PinModeSet(COL_PORT_3, COL_PIN_3, gpioModeInputPull, 1);
}

/**************************************************************************//**
 * @brief
 *    Scan 4x4 keypad for a pressed key
 *
 * @details
 *    Implements a row-column scanning algorithm:
 *    - Drive one row low at a time, keep others high
 *    - Read all column inputs to detect pressed key
 *    - Applies debounce delay to avoid false triggers
 *    - Updates global variables g_lastRow and g_lastCol on detection
 *
 * @return
 *    Returns the ASCII character of the pressed key if detected,
 *    or '\0' if no key is pressed.
 *****************************************************************************/
char keypad_scan(void)
{
  const GPIO_Port_TypeDef row_ports[4] = {ROW_PORT_0, ROW_PORT_1, ROW_PORT_2, ROW_PORT_3};
  const uint8_t row_pins[4] = {ROW_PIN_0, ROW_PIN_1, ROW_PIN_2, ROW_PIN_3};
  const GPIO_Port_TypeDef col_ports[4] = {COL_PORT_0, COL_PORT_1, COL_PORT_2, COL_PORT_3};
  const uint8_t col_pins[4] = {COL_PIN_0, COL_PIN_1, COL_PIN_2, COL_PIN_3};

  // Scan each row
  for (uint8_t r = 0; r < MAX_ROWS; r++) {
      // Drive all rows high
      for (uint8_t i = 0; i < MAX_ROWS; i++) {
          GPIO_PinOutSet(row_ports[i], row_pins[i]);
      }

      // Drive current row low
      GPIO_PinOutClear(row_ports[r], row_pins[r]);
      zdelay_ms(1); // Small delay for signals to stabilize

      // Check each column for a pressed key
      for (uint8_t c = 0; c < MAX_COLS; c++) {
          if (GPIO_PinInGet(col_ports[c], col_pins[c]) == 0) {
              // Debounce
              zdelay_ms(DEBOUNCE_DELAY_MS);
              if (GPIO_PinInGet(col_ports[c], col_pins[c]) == 0) {
                  g_lastRow = r;
                  g_lastCol = c;
                  return key_map[r][c];
              }
          }
      }
  }

  // No key pressed
  return '\0';
}

/**************************************************************************//**
 * @brief
 *    Keypad scan and processing loop
 *
 * @details
 *    This function continuously scans the keypad for key presses and
 *    releases. When a key press is detected, it sends debug information
 *    over UART including the row, column, and detected key. It also
 *    tracks the last key pressed to prevent multiple reports of the
 *    same key while held down.
 *
 *    - Detects rising edge (key press)
 *    - Detects falling edge (key release)
 *
 * @note
 *    Should be called periodically, e.g., from main loop or a timer-driven task.
 *****************************************************************************/
void process_keypad_loop(void)
{
  // Scan the keypad for the currently pressed key
  char key = keypad_scan();

  // Rising edge: new key press detected
  if (key != '\0' && last_key == '\0') {
      last_key = key;

      char buf[128];
      int n = snprintf(buf, sizeof(buf),
                       "[INT] Column change detected\r\n"
                       "Row: %d\r\n"
                       "Col: %d\r\n"
                       "Scan: Row=%d Col=%d -> Key=%c\r\n"
                       "[KEYPAD] Pressed: %c\r\n",
                       g_lastRow, g_lastCol, g_lastRow, g_lastCol, key, key);
      if (n > 0) {
          send_uart_data((const uint8_t *)buf, (size_t)n);
      }
  }
  // Falling edge: key released
  else if (key == '\0' && last_key != '\0') {
      last_key = '\0'; // reset last_key
  }
}

