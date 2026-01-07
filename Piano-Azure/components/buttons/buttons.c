#include "buttons.h"

static const char *TAG = "buttons";

esp_err_t buttons_init(void)
{
    // init GPIO buttons
    gpio_config_t io_conf = 
    {
        .pin_bit_mask = (1ULL<<12)|(1ULL<<13)|(1ULL<<14)|(1ULL<<27),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    }

    //init I2C for PCF8574 
    i2c_config_t i2c_conf = 
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000
    };

    ret = i2c_param_config(I2C_NUM, &i2c_conf);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "i2c_param_config failed: %d", ret);
        return ret;
    }
    ret = i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "i2c_driver_install failed: %d", ret);
        return ret;
    }

    uint8_t dummy=0xFF; 
    printf("Scanning I2C bus...\n");
    for (uint8_t addr = 1; addr < 127; addr++) 
    {
        if (i2c_master_write_to_device(I2C_NUM_0, addr, &dummy, 1, 100 / portTICK_PERIOD_MS) == ESP_OK)
        { 
            printf("Found device at 0x%02X\n", addr); 
        } 
    }

    ESP_LOGI(TAG, "Buttons initialized (4 GPIO + 8 PCF8574)");
    return ESP_OK;
}

uint16_t buttons_read(void)
{
    uint16_t state = 0;

    // read GPIO buttons 
    for(int i=0;i<4;i++)
    {
        //printf("GPIO %d: %d\n", gpio_buttons[i], gpio_get_level(gpio_buttons[i])); //test only
        if(!gpio_get_level(gpio_buttons[i])) // active low
            state |= (1<<i);
    }

    // read PCF8574 buttons 
    uint8_t pcf_data= 0;
    esp_err_t ret  = i2c_master_read_from_device(I2C_NUM, PCF8574_ADDR, &pcf_data, 1, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (ret != ESP_OK) 
    {
        ESP_LOGW(TAG, "PCF8574 read failed: %d", ret);
        return state;
    }

    for(int i=0;i<8;i++)
    {
        if(!(pcf_data & (1 << (7 - i)))) // inverse order: P7->button5 ... P0->button12
            state |= (1 << (i + 4));
    }
    
    return state;
}

bool button_pressed(uint8_t button_id)
{
    if(button_id > 11)  
        return false;
    uint16_t raw = buttons_read();
    return (raw & (1<<button_id)) != 0;
}

void buttons_print(void) 
{
    uint16_t state = buttons_read();
    for(int i = 0; i < 4; i++)
    {
        if((state) & (1 << i))
            printf("Button %d pressed (GPIO %d)\n", i + 1, gpio_buttons[i]);
    }

    for(int i = 0; i < 8; i++)
    {
        int button_num = 5 + i;           
        int pcf_pin = 7 - i;               
        if(state & (1 << (i + 4)))
            printf("Button %d pressed (PCF P%d)\n", button_num, pcf_pin);
    }
}
        