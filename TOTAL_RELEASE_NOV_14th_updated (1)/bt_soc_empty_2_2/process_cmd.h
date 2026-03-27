/*
 * process_cmd.h
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#ifndef PROCESS_CMD_H_
#define PROCESS_CMD_H_

/*!
 *
 * Menu List
 *
 */
typedef struct __menulist{
  uint8_t *cmd;
  uint8_t cmdstr_len;
  void    (*cb)(uint8_t *cbuff, uint16_t len);
}menulist;


void idn_cmd(uint8_t *cmd, uint16_t len);
void swout_high_cmd(uint8_t *cmd, uint16_t len);
void swout_low_cmd(uint8_t *cmd, uint16_t len);
void mem_pwr_on_cmd(uint8_t *cmd, uint16_t len);
void mem_pwr_off_cmd(uint8_t *cmd, uint16_t len);
void gps_en_on_cmd(uint8_t *cmd, uint16_t len);
void gps_en_off_cmd(uint8_t *cmd, uint16_t len);
void gps_reset_on_cmd(uint8_t *cmd, uint16_t len);
void gps_reset_off_cmd(uint8_t *cmd, uint16_t len);
void keypad_on_cmd(uint8_t *cmd, uint16_t len);
void keypad_off_cmd(uint8_t *cmd, uint16_t len);
void gps_on_cmd(uint8_t *cmd, uint16_t len);
void gps_off_cmd(uint8_t *cmd, uint16_t len);
void kn_on_cmd(uint8_t *cmd, uint16_t len);
void kn_off_cmd(uint8_t *cmd, uint16_t len);
void disp_vcc_on_cmd(uint8_t *cmd, uint16_t len);
void disp_vcc_off_cmd(uint8_t *cmd, uint16_t len);
void lcd_test_off_cmd(uint8_t *cmd, uint16_t len);
void lcd_test_on_cmd(uint8_t *cmd, uint16_t len);
void read_dc_cmd(uint8_t *cbuff, uint16_t len);
void read_ac_cmd(uint8_t *cbuff, uint16_t len);
void lcd_back_cmd(uint8_t *cmd, uint16_t len);
void ble_off_cmd(uint8_t *cmd, uint16_t len);
void ble_on_cmd(uint8_t *cmd, uint16_t len);
void adc_sample_length_cmd(uint8_t *cmd, uint16_t len);
void pwr_mode_cmd(uint8_t *cbuff, uint16_t len);
void flash_write_cmd(uint8_t *cmd, uint16_t len);
void flash_read_cmd(uint8_t *cmd, uint16_t len);
void ADC_CLK_RATE(uint8_t *cbuff, uint16_t len);
void enter_em4(void);
void ana_en_off_cmd(uint8_t *cmd, uint16_t len);
void ana_en_on_cmd(uint8_t *cmd, uint16_t len);


#endif /* PROCESS_CMD_H_ */
