#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "display.h"
#include "volume.h"

#define CLK GPIO_NUM_18
#define DT  GPIO_NUM_19
#define SW  GPIO_NUM_23

static const char *TAG = "VOLUME";

static int volume = 50;


int get_volume(void)
{
    return volume;
}


void volume_control(void)
{
    ESP_LOGI("VOLUME", "Entered volume control");

    int previous_clk = gpio_get_level(CLK);

    // Wait for the button press that entered this function
    // to be released.
    while (gpio_get_level(SW) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    ESP_LOGI("VOLUME", "Initial button released");

    int raw_button = 1;
    int stable_button = 1;

    TickType_t button_change_time = xTaskGetTickCount();
    TickType_t last_refresh = xTaskGetTickCount();

    while (1)
    {
        int clk = gpio_get_level(CLK);
        int dt  = gpio_get_level(DT);
        int sw  = gpio_get_level(SW);

        TickType_t now = xTaskGetTickCount();

        // 1. Log button transitions for debugging
        if (sw != raw_button)
        {
            raw_button = sw;
            button_change_time = now;

            ESP_LOGI("VOLUME", "Raw SW changed to %d", sw);
        }

        // 2. Accept a button state after 30 ms of stability
        if (sw == raw_button &&
            (now - button_change_time) >= pdMS_TO_TICKS(30))
        {
            if (stable_button != raw_button)
            {
                stable_button = raw_button;

                if (stable_button == 0)
                {
                    ESP_LOGI("VOLUME", "Button pressed - EXITING");
                    return;
                }
            }
        }

        // 3. Adjust volume using rotary encoder
        if (previous_clk == 1 && clk == 0)
        {
            if (dt == 1 && volume < 100)
                volume++;
            else if (dt == 0 && volume > 0)
                volume--;

            ESP_LOGI("VOLUME", "Current volume: %d", volume);
        }

        previous_clk = clk;

        // 4. Refresh OLED every 250 ms
        if ((now - last_refresh) >= pdMS_TO_TICKS(250))
        {
            volume_page(volume);
            last_refresh = now;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}