#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "display.h"
#include "rotary.h"

#define CLK GPIO_NUM_18
#define DT  GPIO_NUM_19
#define SW  GPIO_NUM_23

volatile page_t selected_page = PAGE_HOME;

void rotary_task(void *pvParameters)
{
    int previous_clk = gpio_get_level(CLK);
    int previous_button = gpio_get_level(SW);

    while (1)
    {
        int clk = gpio_get_level(CLK);
        int dt  = gpio_get_level(DT);
        int sw  = gpio_get_level(SW);

        // Rotate encoder to change pages
        if (previous_clk == 1 && clk == 0)
        {
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

            show_selected_page();
        }

        // Detect button press
        if (previous_button == 1 && sw == 0)
        {
            ESP_LOGI("ROTARY", "Encoder button pressed");
        }

        previous_clk = clk;
        previous_button = sw;

        vTaskDelay(pdMS_TO_TICKS(10));
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