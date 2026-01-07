#pragma once
#include "esp_err.h"         //error codes and macros (ESP_OK, ESP_ERROR_CHECK).
#include <stdint.h>          //fixed-width integer types eg uint8_t
#include <stdbool.h>         //bool types
#include "driver/gpio.h"     //GPIO pins control function
#include "driver/i2c.h"      //I2C communication
#include "esp_log.h"         //logging and debug macros

// GPIO button pins
static const uint8_t gpio_buttons[4] = {13,12,14,27};

// I2C PCF8574
#define PCF8574_ADDR 0x24
#define I2C_NUM I2C_NUM_0
#define SDA_PIN 32
#define SCL_PIN 33
#define BUTTON_COUNT 12
#define I2C_TIMEOUT_MS 500

// initialize all buttons (GPIO + expander PCF8574)
esp_err_t buttons_init(void);

// read button states as 12-bit bitmask
// bit0-3: GPIO buttons (13,12,14,27)
// bit4-11: PCF8574 buttons (P0-P7)
uint16_t buttons_read(void);

// check if a specific button is pressed (0-11)
bool button_pressed(uint8_t button_id);

// print on console
void buttons_print(void);

