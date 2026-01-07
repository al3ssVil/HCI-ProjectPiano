#pragma once
#include "esp_err.h"
#include <stdint.h>
#include "driver/adc.h"
#include "esp_log.h"

// pin ADC
#define POT_ADC_CHANNEL ADC1_CHANNEL_7 // GPIO 35
#define POT_ADC_ATTEN   ADC_ATTEN_DB_11 // 0-3.3V
#define POT_ADC_WIDTH   ADC_WIDTH_BIT_10 // 10 bits (0–1023)
#define POT_MAPPED_MAX  200

// init pin ADC for potensiometer
esp_err_t pot_init(void);

// read value (0 - 1023)
uint16_t pot_read_raw(void);

// read mapped value 0 - 200 (for buzzer)
uint8_t pot_read_mapped(void);
