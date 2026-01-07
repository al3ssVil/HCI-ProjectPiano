idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_http_client json nvs_flash esp_netif esp_event driver lcd buzzer buttons potentiometer esp-tls)