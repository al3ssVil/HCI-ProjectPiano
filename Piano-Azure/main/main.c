#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "lcd.h"
#include "buzzer.h"
#include "potentiometer.h"
#include "buttons.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_tls.h"
#include "cJSON.h"

static SemaphoreHandle_t synth_mutex;

// Function declarations
void wifi_init(void);
esp_err_t send_melody_to_ai(void);

#define LCD_TASK_STACK_SIZE    4096  // Mărit de la 2048
#define LCD_TASK_PRIORITY      3

#define BUZZER_TASK_STACK_SIZE 2048
#define BUZZER_TASK_PRIORITY   3

#define POT_TASK_STACK_SIZE    2048
#define POT_TASK_PRIORITY      2

#define BUTTONS_TASK_STACK_SIZE    2048
#define BUTTONS_TASK_PRIORITY      2

#define RECORD_TASK_STACK_SIZE    4096  // Record task normal
#define RECORD_TASK_PRIORITY      2

// Azure HTTP task (separat pentru stack mai mare)
#define AZURE_TASK_STACK_SIZE     8192  // Stack mare pentru HTTPS/SSL
#define AZURE_TASK_PRIORITY       1

// Record button pin
#define RECORD_BUTTON_GPIO 26

// Recording structures
#define MAX_RECORDED_NOTES 100
typedef struct {
    int note;
    uint32_t frequency;
    uint32_t start_time;    // când începe nota
    uint32_t duration;      // cât timp este ținută apăsată
} recorded_note_t;

static recorded_note_t recorded_melody[MAX_RECORDED_NOTES];
static int recorded_count = 0;
static bool is_recording = false;
static bool is_playing_back = false;
static bool send_to_azure = false;  // Flag pentru Azure task
static uint32_t record_start_time = 0;
static uint32_t current_note_start_time = 0;
static int currently_recording_note = -1;

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

void Record_task(void *pvParameters)
{
    // Initialize record button
    gpio_config_t record_btn_conf = {
        .pin_bit_mask = (1ULL << RECORD_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&record_btn_conf);
    
    bool prev_record_state = false;
    
    while (1)
    {
        bool cur_record_state = !gpio_get_level(RECORD_BUTTON_GPIO); // Active low due to pullup
        
        // Detect button press (rising edge)
        if (cur_record_state && !prev_record_state)
        {
            if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
            {
                if (!is_recording && !is_playing_back)
                {
                    // Start recording
                    is_recording = true;
                    recorded_count = 0;
                    currently_recording_note = -1;
                    record_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                    printf("Recording started...\\n");
                }
                else if (is_recording)
                {
                    // Stop recording and start playback
                    is_recording = false;
                    is_playing_back = true;
                    send_to_azure = true;  // Semnalizează Azure task să trimită
                    printf("Recording stopped. Playing back %d notes...\n", recorded_count);
                }
                else if (is_playing_back)
                {
                    // Stop playback
                    is_playing_back = false;
                    printf("Playback stopped.\n");
                }
                xSemaphoreGive(synth_mutex);
            }
        }
        
        prev_record_state = cur_record_state;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Playback_task(void *pvParameters)
{
    int playback_index = 0;
    uint32_t playback_start_time = 0;
    uint32_t current_note_start = 0;
    bool note_playing = false;
    
    while (1)
    {
        if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
        {
            if (is_playing_back)
            {
                if (playback_index == 0 && playback_start_time == 0)
                {
                    playback_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                }
                
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                uint32_t elapsed = current_time - playback_start_time;
                
                // Check if we need to start a new note
                if (!note_playing && playback_index < recorded_count)
                {
                    if (elapsed >= recorded_melody[playback_index].start_time)
                    {
                        current_note = recorded_melody[playback_index].note;
                        note_changed = true;
                        current_note_start = current_time;
                        note_playing = true;
                        printf("Playing: %s at %luHz for %lums\n", 
                               note_names[recorded_melody[playback_index].note], 
                               recorded_melody[playback_index].frequency,
                               recorded_melody[playback_index].duration);
                    }
                }
                
                // Check if we need to stop the current note
                if (note_playing && playback_index < recorded_count)
                {
                    uint32_t note_elapsed = current_time - current_note_start;
                    if (note_elapsed >= recorded_melody[playback_index].duration)
                    {
                        current_note = -1;
                        note_changed = true;
                        note_playing = false;
                        playback_index++;
                    }
                }
                
                // Check if playback finished
                if (playback_index >= recorded_count && !note_playing)
                {
                    is_playing_back = false;
                    current_note = -1;
                    note_changed = true;
                    playback_index = 0;
                    playback_start_time = 0;
                    printf("Playback finished.\n");
                }
            }
            else
            {
                playback_index = 0;
                playback_start_time = 0;
                note_playing = false;
            }
            xSemaphoreGive(synth_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Buttons_task(void *pvParameters)
{
    buttons_init();
    static bool prev_button_state[12] = {0};

    while (1)
    {
        // Skip button processing during playback
        if (is_playing_back)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        
        bool any_pressed = false;
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

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
                    
                    // Record note start if recording
                    if (is_recording && recorded_count < MAX_RECORDED_NOTES)
                    {
                        float percent = (float)pot_offset / 200.0f * 100.0f;
                        uint32_t freq = note_lower[i] + 
                                        (uint32_t)((note_upper[i] - note_lower[i]) * (percent / 100.0f));
                        
                        recorded_melody[recorded_count].note = i;
                        recorded_melody[recorded_count].frequency = freq;
                        recorded_melody[recorded_count].start_time = current_time - record_start_time;
                        recorded_melody[recorded_count].duration = 0; // will be set when released
                        
                        currently_recording_note = i;
                        current_note_start_time = current_time;
                        printf("Started recording note %d: %s at %luHz\n", recorded_count + 1, note_names[i], freq);
                    }
                    
                    xSemaphoreGive(synth_mutex);
                }
            }
            else if (!cur && prev_button_state[i])
            {
                // Note released
                if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
                {
                    if (is_recording && currently_recording_note == i && recorded_count < MAX_RECORDED_NOTES)
                    {
                        uint32_t note_duration = current_time - current_note_start_time;
                        recorded_melody[recorded_count].duration = note_duration;
                        recorded_count++;
                        currently_recording_note = -1;
                        printf("Finished recording note: %s, duration: %lums\n", note_names[i], note_duration);
                    }
                    
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
    static int playback_note_index = 0;

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
                    uint32_t freq;
                    if (is_playing_back && playback_note_index < recorded_count)
                    {
                        // Use recorded frequency during playback
                        freq = recorded_melody[playback_note_index].frequency;
                        playback_note_index++;
                    }
                    else
                    {
                        // Normal play mode - use potentiometer
                        float percent = (float)pot_offset / 200.0f * 100.0f;
                        freq = note_lower[current_note] + 
                                    (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                    }
                    buzzer_play(freq);
                }
            }
            else if (current_note != -1 && pot_offset != last_offset && !is_playing_back)
            {
                float percent = (float)pot_offset / 200.0f * 100.0f;
                uint32_t freq = note_lower[current_note] + 
                                (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                buzzer_play(freq);
            }

            // Reset playback index when playback stops
            if (!is_playing_back)
            {
                playback_note_index = 0;
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
    bool last_recording = false;
    bool last_playing = false;

    while (1)
    {
        if (xSemaphoreTake(synth_mutex, portMAX_DELAY))
        {
            if (current_note != last_note || pot_offset != last_offset || 
                is_recording != last_recording || is_playing_back != last_playing)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);

                // Show recording/playback status
                if (is_recording)
                {
                    char buf[20];
                    snprintf(buf, sizeof(buf), "REC %d notes", recorded_count);
                    lcd_print(buf);
                }
                else if (is_playing_back)
                {
                    char buf[20];
                    snprintf(buf, sizeof(buf), "PLAY %d notes", recorded_count);
                    lcd_print(buf);
                }
                else if (current_note == -1)
                {
                    if (recorded_count > 0)
                    {
                        char buf[20];
                        snprintf(buf, sizeof(buf), "Ready (%d)", recorded_count);
                        lcd_print(buf);
                    }
                    else
                    {
                        lcd_print("Piano Ready");
                    }
                }
                else
                {
                    lcd_print("Note: ");
                    lcd_print(note_names[current_note]);
                }

                // Second line
                lcd_set_cursor(0, 1);
                if (current_note != -1 && !is_recording && !is_playing_back)
                {
                    float percent = (float)pot_offset / 200.0f * 100.0f;
                    uint32_t freq = note_lower[current_note] +
                                    (uint32_t)((note_upper[current_note] - note_lower[current_note]) * (percent / 100.0f));
                    char buf[20];
                    snprintf(buf, sizeof(buf), "%luHz %+ld", note_freqs[current_note], (long)(freq - note_freqs[current_note]));
                    lcd_print(buf);
                }
                else
                {
                    lcd_print("GPIO26=Rec/Play");
                }

                last_note = current_note;
                last_offset = pot_offset;
                last_recording = is_recording;
                last_playing = is_playing_back;
            }
            xSemaphoreGive(synth_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Azure HTTP Task - stack mare pentru HTTPS/SSL
void Azure_task(void *pvParameters)
{
    while (1) {
        if (send_to_azure) {
            if (xSemaphoreTake(synth_mutex, portMAX_DELAY)) {
                send_to_azure = false;  // Reset flag
                xSemaphoreGive(synth_mutex);
                
                // Send melody to AI Assistant
                printf("Sending melody to AI Assistant...\n");
                if (send_melody_to_ai() == ESP_OK) {
                    printf("Melody sent successfully!\n");
                } else {
                    printf("Failed to send melody to AI\n");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // Check every 500ms
    }
}

void app_main(void)
{
    synth_mutex = xSemaphoreCreateMutex();
    
    // Initialize certificates for HTTPS
    esp_tls_init_global_ca_store();
    
    // Initialize WiFi
    wifi_init();
    
    xTaskCreate(Buttons_task, "Buttons Task", BUTTONS_TASK_STACK_SIZE, NULL, BUTTONS_TASK_PRIORITY, NULL);
    xTaskCreate(Pot_task, "Potentiometer Task", POT_TASK_STACK_SIZE, NULL, POT_TASK_PRIORITY, NULL);
    xTaskCreate(Buzzer_task, "Buzzer Task", BUZZER_TASK_STACK_SIZE, NULL, BUZZER_TASK_PRIORITY, NULL);
    xTaskCreate(LCD_task, "LCD Task", LCD_TASK_STACK_SIZE, NULL, LCD_TASK_PRIORITY, NULL);
    xTaskCreate(Record_task, "Record Task", RECORD_TASK_STACK_SIZE, NULL, RECORD_TASK_PRIORITY, NULL);
    xTaskCreate(Playback_task, "Playback Task", RECORD_TASK_STACK_SIZE, NULL, RECORD_TASK_PRIORITY, NULL);
    xTaskCreate(Azure_task, "Azure Task", AZURE_TASK_STACK_SIZE, NULL, AZURE_TASK_PRIORITY, NULL);
}

// WiFi credentials - COMPLETEAZĂ CU DATELE TALE!
#define WIFI_SSID "Nicole"      // Pune aici numele rețelei tale WiFi
#define WIFI_PASS "20042005"          // Pune aici parola rețelei tale WiFi

// API endpoint
#define API_URL "https://hciaznicollab7-c5geawdqd4csdzf4.germanywestcentral-01.azurewebsites.net/api"

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        printf("Reconnecting to WiFi...\n");
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("WiFi connected successfully!\n");
    }
}

// Initialize WiFi
void wifi_init(void)
{
    printf("Starting WiFi initialization...\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    printf("Connecting to WiFi: %s\n", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("WiFi initialization completed.\n");
}

// Send recorded melody to AI Assistant
esp_err_t send_melody_to_ai(void)
{
    if (recorded_count == 0) return ESP_FAIL;
    
    // Create JSON with note sequence
    cJSON *json = cJSON_CreateObject();
    cJSON *deviceId = cJSON_CreateString("piano_esp32");
    
    // Create notes sequence string
    char notes_sequence[256] = "";
    for (int i = 0; i < recorded_count && i < 20; i++) { // Limit to 20 notes
        strcat(notes_sequence, note_names[recorded_melody[i].note]);
        if (i < recorded_count - 1) strcat(notes_sequence, "-");
    }
    
    char message[512];
    snprintf(message, sizeof(message), "Melody recorded: %s (Total %d notes)", notes_sequence, recorded_count);
    
    cJSON *messageJson = cJSON_CreateString(message);
    cJSON *sensorType = cJSON_CreateString("melody");
    cJSON *value = cJSON_CreateNumber(recorded_count);
    cJSON *unit = cJSON_CreateString("notes");
    
    cJSON_AddItemToObject(json, "deviceId", deviceId);
    cJSON_AddItemToObject(json, "message", messageJson);
    cJSON_AddItemToObject(json, "sensorType", sensorType);
    cJSON_AddItemToObject(json, "value", value);
    cJSON_AddItemToObject(json, "unit", unit);
    
    char *json_string = cJSON_Print(json);
    
    // HTTP request
    esp_http_client_config_t config = {
        .url = API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .disable_auto_redirect = false,
        .max_redirection_count = 3,
        .is_async = false,
        .cert_pem = NULL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        printf("HTTP POST Status = %d\n", status);
        
        // Read response
        char response_buffer[512];
        int content_length = esp_http_client_read(client, response_buffer, sizeof(response_buffer) - 1);
        if (content_length > 0) {
            response_buffer[content_length] = '\0';
            printf("AI Response: %s\n", response_buffer);
            
            // Parse and display song name on LCD
            cJSON *response_json = cJSON_Parse(response_buffer);
            if (response_json) {
                cJSON *ai_response = cJSON_GetObjectItem(response_json, "response");
                if (ai_response && cJSON_IsString(ai_response)) {
                    // Display on LCD (you'll need to modify LCD task to show this)
                    printf("Detected song: %s\n", ai_response->valuestring);
                }
                cJSON_Delete(response_json);
            }
        }
    }
    
    esp_http_client_cleanup(client);
    free(json_string);
    cJSON_Delete(json);
    
    return err;
}