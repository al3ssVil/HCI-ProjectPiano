#pragma once
#include "esp_err.h"
#include <stdint.h>
#include "driver/ledc.h"
#include "esp_log.h"

#define BUZZER_GPIO 25
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0
#define BUZZER_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BUZZER_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define BUZZER_MAX_DUTY        1023

// init buzzer
esp_err_t buzzer_init(void);

// play a tone with specified frequency 
esp_err_t buzzer_play(uint32_t freq_hz);

// stop
esp_err_t buzzer_stop(void);