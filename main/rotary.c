#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "display.h"
#include "rotary.h"
#include "volume.h"
#include "command.h"

#define CLK GPIO_NUM_18
#define DT  GPIO_NUM_19
#define SW  GPIO_NUM_23

volatile page_t selected_page = PAGE_HOME;

void rotary_task(void *pvParameters)
{
    int previous_clk = gpio_get_level(CLK);

    int button_reading = gpio_get_level(SW);
    int stable_button = button_reading;

    TickType_t last_button_change = xTaskGetTickCount();
    TickType_t last_refresh = xTaskGetTickCount();

    while (1)
    {
        int clk = gpio_get_level(CLK);
        int dt  = gpio_get_level(DT);
        int sw  = gpio_get_level(SW);

        TickType_t now = xTaskGetTickCount();

        // ------------------------------------------
        // 1. Detect rotary encoder movement
        // ------------------------------------------
        if (previous_clk == 1 && clk == 0)
        {
            ESP_LOGI("ROTARY", "CLK=%d DT=%d", clk, dt);

            if (dt == 1)
            {
                selected_page =
                    (selected_page + 1) % PAGE_COUNT;
            }
            else
            {
                selected_page =
                    (selected_page + PAGE_COUNT - 1) % PAGE_COUNT;
            }

            ESP_LOGI("ROTARY", "Selected page: %d",
                     (int)selected_page);
        }

        previous_clk = clk;

        // ------------------------------------------
        // 2. Debounce the encoder button
        // ------------------------------------------
        if (sw != button_reading)
        {
            button_reading = sw;
            last_button_change = now;
        }

        if ((now - last_button_change) >= pdMS_TO_TICKS(30))
        {
            if (stable_button != button_reading)
            {
                stable_button = button_reading;

                // Button is active-low
                if (stable_button == 0)
                {
                    switch (selected_page)
                    {
                        case PAGE_VOLUME:
                            ESP_LOGI("ROTARY",
                                     "Entering volume control");

                            // Control returns here when the button
                            // is pressed again inside volume_control()
                            volume_control();

                            ESP_LOGI("ROTARY",
                                     "Returned to page navigation");

                            // Reinitialize input states
                            previous_clk = gpio_get_level(CLK);
                            button_reading = gpio_get_level(SW);
                            stable_button = button_reading;

                            last_button_change =
                                xTaskGetTickCount();

                            last_refresh =
                                xTaskGetTickCount();
                            break;

                        case PAGE_MEDIA:
                            ESP_LOGI("ROTARY",
                                     "Media button pressed");
                            break;

                        case PAGE_LOCK:
                            ESP_LOGI("ROTARY",
                                     "Lock button pressed");
                            commands_request_lock();
                            break;

                        case PAGE_HOME:
                        case PAGE_PROCESSES:
                        default:
                            // No button action on these pages
                            break;
                    }
                }
            }
        }

        // ------------------------------------------
        // 3. Refresh OLED every 250 ms
        // ------------------------------------------
        now = xTaskGetTickCount();

        if ((now - last_refresh) >= pdMS_TO_TICKS(250))
        {
            show_selected_page();
            last_refresh = now;
        }

        // ------------------------------------------
        // 4. Poll every 2 ms
        // ------------------------------------------
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void rotary_init(void)
{
    gpio_config_t encoder_config = {
        .pin_bit_mask = (1ULL << CLK) |
                        (1ULL << DT)  |
                        (1ULL << SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&encoder_config));
}