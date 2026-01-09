#include "buzzer.h"

static const char *TAG = "buzzer";

esp_err_t buzzer_init(void) 
{
    ledc_timer_config_t timer_conf = 
    {
        .speed_mode = BUZZER_LEDC_MODE,
        .timer_num = BUZZER_LEDC_TIMER,
        .duty_resolution = BUZZER_DUTY_RESOLUTION,
        .freq_hz = 262,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = 
    {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0, // stop
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);

    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_GPIO);
    return ESP_OK;
}
// to play between 262 and 684 for the sound to not be distorted or weaker
esp_err_t buzzer_play(uint32_t freq_hz) 
{
    if(freq_hz < 262) freq_hz = 262;
    if(freq_hz > 694) freq_hz = 694;

    ledc_set_freq(BUZZER_LEDC_MODE, BUZZER_LEDC_TIMER, freq_hz);
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_MAX_DUTY / 2); // half of 10-bit
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
    return ESP_OK;
}

esp_err_t buzzer_stop(void) 
{
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
    return ESP_OK;
}
