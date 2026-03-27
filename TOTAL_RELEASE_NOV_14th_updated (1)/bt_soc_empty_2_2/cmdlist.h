/*
 * cmdlist.h
 *
 *  Created on: 08-july-2025
 *      Author: Bavatharani
 */

#ifndef CMDLIST_H_
#define CMDLIST_H_

/*!
 * Command Menu list
 */
menulist ml[] =
    {
        {(uint8_t *)"*IDN?"                             ,5,  idn_cmd},
        {(uint8_t *)"RELAY:ON=KCH1"                     ,13, sr_ch1_relay_on },
        {(uint8_t *)"RELAY:OFF=KCH1"                    ,14, sr_ch1_relay_off },
        {(uint8_t *)"RELAY:ON=KCH2"                     ,13, sr_ch2_relay_on },
        {(uint8_t *)"RELAY:OFF=KCH2"                    ,14, sr_ch2_relay_off },
        {(uint8_t *)"RELAY:ON=KCH3"                     ,13, sr_ch3_relay_on },
        {(uint8_t *)"RELAY:OFF=KCH3"                    ,14, sr_ch3_relay_off },
        {(uint8_t *)"RELAY:ON=KCH4"                     ,13, sr_ch4_relay_on },
        {(uint8_t *)"RELAY:OFF=KCH4"                    ,14, sr_ch4_relay_off },
        {(uint8_t *)"RELAY:ON=KCH4S"                    ,13, sr_ch4S_relay_on },
        {(uint8_t *)"RELAY:OFF=KCH4S"                   ,14, sr_ch4S_relay_off },
        {(uint8_t *)"RELAY:ON=KCRNG1"                   ,15, sr_kcrng1_relay_on },
        {(uint8_t *)"RELAY:OFF=KCRNG1"                  ,16, sr_kcrng1_relay_off },
        {(uint8_t *)"RELAY:ON=KCRNG2"                   ,15, sr_kcrng2_relay_on },
        {(uint8_t *)"RELAY:OFF=KCRNG2"                  ,16,sr_kcrng2_relay_off },
        {(uint8_t *)"RELAY:ON=KCRNG4"                   ,15,sr_kcrng4_relay_on },
        {(uint8_t *)"RELAY:OFF=KCRNG4"                  ,16,sr_kcrng4_relay_off } ,
        {(uint8_t *)"RELAY:ON=KCAL"                     ,13, sr_kcal_relay_on },
        {(uint8_t *)"RELAY:OFF=KCAL"                    ,14, sr_kcal_relay_off },
        {(uint8_t *)"CONF:ADS124"                       ,11,  ads124_conf_adc },
        {(uint8_t *)"INP:ADS124"                        ,10,  ads124_inp_adc },
        {(uint8_t *)"SWOUT=HIGH"                        ,10, swout_high_cmd },
        {(uint8_t *)"SWOUT=LOW"                         ,9, swout_low_cmd },
        {(uint8_t *)"ANA_EN=ON"                        ,9,ana_en_on_cmd },
        {(uint8_t *)"ANA_EN=OFF"                         ,10, ana_en_off_cmd },
        {(uint8_t *)"MEM_PWR=ON"                        ,10, mem_pwr_on_cmd },
        {(uint8_t *)"MEM_PWR=OFF"                       ,11, mem_pwr_off_cmd },
        {(uint8_t *)"DISP_VCC=ON"                       ,11, disp_vcc_on_cmd },
        {(uint8_t *)"DISP_VCC=OFF"                      ,12, disp_vcc_off_cmd },
        {(uint8_t *)"GPS_EN=ON"                         ,9, gps_en_on_cmd },
        {(uint8_t *)"GPS_EN=OFF"                        ,10, gps_en_off_cmd },
        {(uint8_t *)"GPS_RESET=ON"                      ,12, gps_reset_on_cmd },
        {(uint8_t *)"GPS_RESET=OFF"                     ,13, gps_reset_off_cmd },
        {(uint8_t *)"KEYPAD=ON"                         ,9, keypad_on_cmd },
        {(uint8_t *)"KEYPAD=OFF"                        ,10, keypad_off_cmd },
        {(uint8_t *)"GPS=ON"                            ,6,  gps_on_cmd },
        {(uint8_t *)"GPS=OFF"                           ,7, gps_off_cmd },
        { (uint8_t *)"READ_DC0"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_DC1"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_DC2"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_DC3"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_DC4"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_DC5"                         ,8, read_dc_cmd },
        { (uint8_t *)"READ_AC0"                         ,8, read_ac_cmd },
        { (uint8_t *)"READ_AC1"                         ,8, read_ac_cmd },
        { (uint8_t *)"READ_AC2"                         ,8, read_ac_cmd },
        { (uint8_t *)"READ_AC3"                         ,8, read_ac_cmd },
        { (uint8_t *)"READ_AC4"                         ,8, read_ac_cmd },
        { (uint8_t *)"READ_AC5"                         ,8, read_ac_cmd },
        { (uint8_t *)"ADC_TYPE="                        ,9, adc_type_cmd },
        {(uint8_t *)"LCD_TEST=ON"                       ,11, lcd_test_on_cmd },
        {(uint8_t *)"LCD_TEST=OFF"                      ,12, lcd_test_off_cmd },
        { (uint8_t *)"LCD_BACK="                        ,9, lcd_back_cmd },
        {(uint8_t *)"BLE=ON"                            ,6, ble_on_cmd},
        {(uint8_t *)"BLE=OFF"                           ,7, ble_off_cmd},
        {(uint8_t *)"ADC_SAMPLE_LENGTH="                ,18, adc_sample_length_cmd},
        { (uint8_t *)"PWR_MODE="                        ,9, pwr_mode_cmd },
        {(uint8_t *)"FLASH_WRITE="                      ,12, flash_write_cmd },
        {(uint8_t *)"FLASH_READ="                       ,11, flash_read_cmd },
        {(uint8_t *)"ADC_CLK_RATE="                     ,13, ADC_CLK_RATE}
    };

#endif /* CMDLIST_H_ */
