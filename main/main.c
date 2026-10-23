#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "display.h"
#include "rotary.h"
#include "wifi.h"

void app_main(void)
{
    I2C_init();
    oled_init();
    clear_framebuffer();
    rotary_init();
    home(info.cpu, info.ram, info.gpu);
    wifi_init();
    xTaskCreate(rotary_task, "rotary_task", 4096, NULL, 5, NULL);
}