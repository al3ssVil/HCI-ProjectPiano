#include "potentiometer.h"

static const char *TAG = "pot";

esp_err_t pot_init(void)
{
    adc1_config_width(POT_ADC_WIDTH);
    adc1_config_channel_atten(POT_ADC_CHANNEL, POT_ADC_ATTEN);

    ESP_LOGI(TAG, "Potentiometer initialized on GPIO 35 (ADC1_CH7)");
    return ESP_OK;
}

uint16_t pot_read_raw(void)
{
    return adc1_get_raw(POT_ADC_CHANNEL); // 0–1023
}

uint8_t pot_read_mapped(void)
{
    uint16_t raw10 = pot_read_raw();          // 0–1023
    uint8_t offset = (raw10 * 200) / 1023;         // 0–200
    return offset;
}
