idf_component_register(
    SRCS
        "main.c"
        "oled.c"
        "display.c"
        "rotary.c"
        "wifi.c"
        "http.c"

    INCLUDE_DIRS "."

    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
)
