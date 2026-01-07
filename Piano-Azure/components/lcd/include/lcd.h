#pragma once
#include "esp_err.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <unistd.h>  // for usleep


#define LCD_RS_PIN  4
#define LCD_E_PIN   5
#define LCD_D4_PIN  18
#define LCD_D5_PIN  19
#define LCD_D6_PIN  21
#define LCD_D7_PIN  22
#define LCD_CMD_FUNCTION_RESET   0x03  // reset sequence command 
#define LCD_CMD_FUNCTION_4BIT    0x02  // set 4-bit interface
#define LCD_CMD_FUNCTION_SET     0x28  // 4-bit, 2 lines, 5x8 font
#define LCD_CMD_DISPLAY_CONTROL  0x0C  // display ON, cursor OFF, blink OFF
#define LCD_CMD_ENTRY_MODE       0x06  // cursor moves right, no shift
#define LCD_CMD_CLEAR_DISPLAY    0x01  // clear screen, return home
#define LCD_CMD_SET_DDRAM_ADDR   0x80  // base address pentru cursor

#define LCD_DELAY_US 50

// LCD-ul init
esp_err_t lcd_init(void);

// write text on LCD
esp_err_t lcd_print(const char *text);

// set cursor
esp_err_t lcd_set_cursor(uint8_t col, uint8_t row);

// clear screen
esp_err_t lcd_clear(void);
