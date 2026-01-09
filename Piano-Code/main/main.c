#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_http_server.h"  // for HTTP server and SSE
#include "esp_wifi.h"         // for WiFi
#include "esp_event.h"        // for event loop
#include "esp_netif.h"        // for network interface
#include "nvs_flash.h"        // for NVS flash
#include "esp_mac.h"          // for MAC address
#include "lwip/ip4_addr.h"    // for IP address conversion

#include "lcd.h"
#include "buzzer.h"
#include "potentiometer.h"
#include "buttons.h"

#define MAX_CLIENTS 4

typedef struct {
    httpd_req_t *req;
    bool active;
} sse_client_t;

static sse_client_t sse_clients[MAX_CLIENTS] = {0};
static SemaphoreHandle_t sse_mutex;
static SemaphoreHandle_t synth_mutex;

#define LCD_TASK_STACK_SIZE    2048
#define LCD_TASK_PRIORITY      3

#define BUZZER_TASK_STACK_SIZE 2048
#define BUZZER_TASK_PRIORITY   3

#define POT_TASK_STACK_SIZE    2048
#define POT_TASK_PRIORITY      2

#define BUTTONS_TASK_STACK_SIZE    2048
#define BUTTONS_TASK_PRIORITY      2

static const char *note_names[12] = {
    "C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4"
};

static const uint32_t note_freqs[12] = {
    261, 277, 293, 311, 329, 349, 369, 392, 415, 440, 466, 493
};
  
static const uint32_t note_lower[12] = {
    261, 269, 285, 302, 320, 339, 359, 380, 403, 428, 453, 480    
};

static const uint32_t note_upper[12] = {
    269, 285, 302, 320, 339, 359, 380, 403, 428, 453, 480, 523      
};

static volatile int current_note = -1;  
static volatile uint16_t pot_offset = 0;
static volatile bool note_changed = false;

// send message to all clients 
static void sse_send_all(const char *msg) 
{
    xSemaphoreTake(sse_mutex, portMAX_DELAY);

    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (sse_clients[i].active && sse_clients[i].req) 
        {
            char buf[128];
            int len = snprintf(buf, sizeof(buf), "data: %s\n\n", msg);
            httpd_resp_send_chunk(sse_clients[i].req, buf, len);
        }
    }

    xSemaphoreGive(sse_mutex);
}

esp_err_t sse_handler(httpd_req_t *req) 
{
  httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    // add client in list
    xSemaphoreTake(sse_mutex, portMAX_DELAY);
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (!sse_clients[i].active) 
        {
            sse_clients[i].req = req;
            sse_clients[i].active = true;
            slot = i;
            break;
        }
    }
    xSemaphoreGive(sse_mutex);

    if (slot == -1) 
    {
        return ESP_FAIL;
    }

    // send init message
    const char *init_msg = "data: connected\n\n";
    httpd_resp_send_chunk(req, init_msg, strlen(init_msg));

    printf("SSE client connected on slot %d\n", slot);

    while (1) 
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        const char *hb = ":\n\n"; // valid comm in SSE
        esp_err_t res = httpd_resp_send_chunk(req, hb, strlen(hb));
        if (res != ESP_OK) 
        {
            printf("SSE client %d disconnected, error: %d\n", slot, res);
            break;
        }
    }

    // delete client when disconnected
    xSemaphoreTake(sse_mutex, portMAX_DELAY);
    sse_clients[slot].active = false;
    sse_clients[slot].req = NULL;
    xSemaphoreGive(sse_mutex);

    return ESP_OK;
}

void start_sse_server(void) 
{
    sse_mutex = xSemaphoreCreateMutex();

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);
    
    httpd_uri_t sse_uri = 
    {
        .uri = "/sse", // endpoint SSE
        .method = HTTP_GET,
        .handler = sse_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &sse_uri);

    printf("SSE server started at /sse\n");
}

void wifi_init_sta(void)
{
    esp_netif_init();          // init TCP/IP stack
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = "GalaxyA71",
            .password = "qwerty12",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void wait_for_ip(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    while (esp_netif_get_ip_info(sta_netif, &ip_info) != ESP_OK || ip_info.ip.addr == 0) 
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void Buttons_task(void *pvParameters)
{
    buttons_init();
    static bool prev_button_state[12] = {0};

    while (1)
    {
        bool any_pressed = false;

        for (int i = 0; i < 12; i++)
        {
            bool cur = button_pressed(i);

            if (cur && !prev_button_state[i])
            {
                if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
                {
                    current_note = i;
                    note_changed = true;
                    printf("%s\n", note_names[i]);
        char msg[64];
        sprintf(msg, "note_on:%d\n\n", current_note);
        sse_send_all(msg);
                    xSemaphoreGive(synth_mutex);
                }
            }
            prev_button_state[i] = cur;
            if (cur) any_pressed = true;
        }

        if (!any_pressed && current_note != -1)
        {
            if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
            {
                current_note = -1;
                note_changed = true;
        char msg[64];
        sprintf(msg, "note_on:%d\n\n", current_note);
        sse_send_all(msg);
                xSemaphoreGive(synth_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Pot_task(void *pvParameters)
{
    pot_init();

    while (1)
    {
        uint16_t new_offset = pot_read_mapped(); // 0 - 200

        if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
        {
            pot_offset = new_offset;
            xSemaphoreGive(synth_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void Buzzer_task(void *pvParameters)
{
    buzzer_init();
    uint16_t last_offset = 0;

    while (1)
    {
        if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
        {
            if (note_changed)
            {
                note_changed = false;

                if (current_note == -1)
                {
                    buzzer_stop();
                }
                else
                {
                    float percent = (float)pot_offset / 200.0f * 100.0f;; 
                    uint32_t freq = note_lower[current_note] + 
                                (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                    buzzer_play(freq);
                }
            }
            else if (current_note != -1 && pot_offset != last_offset)
            {
                float percent = (float)pot_offset / 200.0f * 100.0f;; 
                uint32_t freq = note_lower[current_note] + 
                                (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                buzzer_play(freq);
            }

            last_offset = pot_offset;
            xSemaphoreGive(synth_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void LCD_task(void *pvParameters)
{
    lcd_init();
    lcd_clear();

    int last_note = -1;
    uint16_t last_offset = 0;

    while (1)
    {
        if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
        {
            if (current_note != last_note || pot_offset != last_offset)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);

                if (current_note == -1)
                {
                    lcd_print("No key pressed");
                }
                else
                {
                    float percent = (float)pot_offset / 200.0f * 100.0f;
                    uint32_t freq = note_lower[current_note] +
                                    (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                    lcd_print("Note: ");
                    lcd_print(note_names[current_note]);
                    lcd_set_cursor(0, 1);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%luHz %+ld", note_freqs[current_note], (long)(freq - note_freqs[current_note]));
                    lcd_print(buf);
                }

                last_note = current_note;
                last_offset = pot_offset;
            }
            xSemaphoreGive(synth_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    nvs_flash_init();
    wifi_init_sta();
    wait_for_ip();
    vTaskDelay(pdMS_TO_TICKS(5000));

    synth_mutex = xSemaphoreCreateMutex();
    sse_mutex = xSemaphoreCreateMutex();

    esp_netif_ip_info_t ipInfo;
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (esp_netif_get_ip_info(sta_netif, &ipInfo) == ESP_OK) {
        char ipStr[16];
        ip4addr_ntoa_r((const ip4_addr_t *)&ipInfo.ip, ipStr, sizeof(ipStr));
        printf("ESP32 IP: %s\n", ipStr);
    }

    start_sse_server();

    xTaskCreate(Buttons_task, "Buttons Task", BUTTONS_TASK_STACK_SIZE, NULL, BUTTONS_TASK_PRIORITY, NULL);
    xTaskCreate(Pot_task, "Potentiometer Task", POT_TASK_STACK_SIZE, NULL, POT_TASK_PRIORITY, NULL);
    xTaskCreate(Buzzer_task, "Buzzer Task", BUZZER_TASK_STACK_SIZE, NULL, BUZZER_TASK_PRIORITY, NULL);
    xTaskCreate(LCD_task, "LCD Task", LCD_TASK_STACK_SIZE, NULL, LCD_TASK_PRIORITY, NULL);
}