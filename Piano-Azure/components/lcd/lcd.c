#include "lcd.h"

static const char *TAG = "lcd";

// send 4 bits
static void lcd_send_nibble(uint8_t nibble) 
{
    gpio_set_level(LCD_D4_PIN, (nibble >> 0) & 0x01);
    gpio_set_level(LCD_D5_PIN, (nibble >> 1) & 0x01);
    gpio_set_level(LCD_D6_PIN, (nibble >> 2) & 0x01);
    gpio_set_level(LCD_D7_PIN, (nibble >> 3) & 0x01);

    // Trigger E
    gpio_set_level(LCD_E_PIN, 1);
    esp_rom_delay_us(LCD_DELAY_US);
    gpio_set_level(LCD_E_PIN, 0);
    esp_rom_delay_us(LCD_DELAY_US);
}

// send command or data
static void lcd_send_byte(uint8_t byte, uint8_t is_data) 
{
    gpio_set_level(LCD_RS_PIN, is_data);
    lcd_send_nibble(byte >> 4);  // high nibble
    lcd_send_nibble(byte & 0x0F); // low nibble
}

esp_err_t lcd_init(void)
 {
    // config pin
    gpio_config_t io_conf = 
    {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<LCD_RS_PIN)|(1ULL<<LCD_E_PIN)|
                        (1ULL<<LCD_D4_PIN)|(1ULL<<LCD_D5_PIN)|
                        (1ULL<<LCD_D6_PIN)|(1ULL<<LCD_D7_PIN)
    };
    gpio_config(&io_conf);

    esp_rom_delay_us(40000); // wait 40ms after power on

    // init LCD 4-bit
    lcd_send_nibble(LCD_CMD_FUNCTION_RESET);
    esp_rom_delay_us(4500);
    lcd_send_nibble(LCD_CMD_FUNCTION_RESET);
    esp_rom_delay_us(4500);
    lcd_send_nibble(LCD_CMD_FUNCTION_RESET);
        esp_rom_delay_us(150);
    lcd_send_nibble(LCD_CMD_FUNCTION_4BIT); // 4-bit mode

    // config display: 2 lines, 5x8, display on, cursor off
    lcd_send_byte(LCD_CMD_FUNCTION_SET, 0); // function set
    lcd_send_byte(LCD_CMD_DISPLAY_CONTROL, 0); // display on/off control
    lcd_send_byte(LCD_CMD_ENTRY_MODE, 0); // entry mode set
    lcd_clear();

    ESP_LOGI(TAG, "LCD initialized in 4-bit mode");
    return ESP_OK;
}

esp_err_t lcd_clear(void) 
{
    lcd_send_byte(LCD_CMD_CLEAR_DISPLAY, 0);
    esp_rom_delay_us(2000); 
    return ESP_OK;
}

esp_err_t lcd_set_cursor(uint8_t col, uint8_t row) 
{
    uint8_t addr = LCD_CMD_SET_DDRAM_ADDR + (row * 0x40 + col);
    lcd_send_byte(addr, 0);
    return ESP_OK;
}

esp_err_t lcd_print(const char *text) 
{
    while (*text) {
        lcd_send_byte(*text++, 1);
    }
    return ESP_OK;
}
