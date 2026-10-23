#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "oled.h"


#define SDA GPIO_NUM_21
#define SCL GPIO_NUM_22
#define OLED_ADDRESS 0x3C
#define OLED_I2C_FREQ 100000


#define WIDTH 128
#define HEIGHT 64
#define PAGES 8
#define BUFFER 128*8


static const uint8_t font[][5] = {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // space (index 0 = ASCII 32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x56, 0x20, 0x50}, // &
    {0x00, 0x08, 0x07, 0x03, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x80, 0x70, 0x30, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x00, 0x60, 0x60, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x72, 0x49, 0x49, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x49, 0x4D, 0x33}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x31}, // 6
    {0x41, 0x21, 0x11, 0x09, 0x07}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x46, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x59, 0x09, 0x06}, // ?
    {0x3E, 0x41, 0x5D, 0x59, 0x4E}, // @
    // A
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    // B
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    // C
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    // D
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    // E
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    // F
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    // G
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    // H
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    // I
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    // J
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    // K
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    // L
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    // M
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    // N
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    // O
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    // P
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    // Q
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    // R
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    // S
    {0x46, 0x49, 0x49, 0x49, 0x31},
    // T
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    // U
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    // V
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    // W
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    // X
    {0x63, 0x14, 0x08, 0x14, 0x63},
    // Y
    {0x07, 0x08, 0x70, 0x08, 0x07},
    // Z
    {0x61, 0x51, 0x49, 0x45, 0x43}
};
static uint8_t frameBuffer[8][128];
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t device_handle;


void I2C_init()
{
    i2c_master_bus_config_t bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = SCL,
    .sda_io_num = SDA,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t i2c_device = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = OLED_ADDRESS,
    .scl_speed_hz = OLED_I2C_FREQ,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle,&i2c_device,&device_handle));
}


void send_command(i2c_master_dev_handle_t device_handle, uint8_t command)
{
    uint8_t cmd[2] = {0x00, command};
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, cmd, sizeof(cmd), -1));
}


void oled_init()
{
    uint8_t oled_init_commands[] = {
    0xAE,       // Display OFF
    0xD5, 0x80, // Display clock divide ratio / oscillator frequency
    0xA8, 0x3F, // Multiplex ratio: 64 COM lines
    0xD3, 0x00, // Display offset: 0
    0x40,       // Display start line: 0
    0x8D, 0x14, // Enable charge pump
    0x20, 0x02, // Page addressing mode
    0xA1,       // Segment remap
    0xC8,       // COM output scan direction
    0xDA, 0x12, // COM pin hardware configuration
    0x81, 0x7F, // Contrast
    0xD9, 0xF1, // Pre-charge period
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // Display follows GDDRAM contents
    0xA6,       // Normal display (not inverted)
    0xAF        // Display ON
    };
    for(int i = 0; i < sizeof(oled_init_commands)/sizeof(oled_init_commands[0]); i++)
    {
        send_command(device_handle, oled_init_commands[i]);
    }
}


void send_data(i2c_master_dev_handle_t device_handle, uint8_t* framebuffer_, size_t length)
{
    uint8_t* buffer = malloc(length + 1);
    buffer[0] = 0x40;
    memcpy(buffer + 1, framebuffer_, length);
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, buffer, length + 1, -1));
    free(buffer);
}


void pixel(int x, int y, bool on)
{
    if(y < 0 || y >= HEIGHT || x < 0 || x >= WIDTH)
        return;

    if(on)
        frameBuffer[y/8][x] |= (1 << (y%8));
    else
        frameBuffer[y/8][x] &=~(1 << (y%8));
}


void clear_framebuffer()
{
    memset(frameBuffer, 0, sizeof(frameBuffer));
}


void display_framebeffer()
{
    for (int i = 0;i<PAGES;i++)
    {
        send_command(device_handle, 0xB0 + i);
        send_command(device_handle, 0x00);
        send_command(device_handle, 0x10);
        send_data(device_handle, frameBuffer[i], WIDTH);
    }
}


void draw_line(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int error = dx - dy;

    while (1)
    {
        pixel(x0, y0, true);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * error;
        if (e2 > -dy)
        {
            error -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}


void draw_rectangle(int x0, int y0, int x1, int y1)
{
    draw_line(x0, y0, x1, y0);
    draw_line(x1, y0, x1, y1);
    draw_line(x1, y1, x0, y1);
    draw_line(x0, y1, x0, y0);
}

void draw_char(int x, int y, char c)
{
    if (c >= 'a'  && c <= 'z')
        c = c- 32;
    if (c < ' ' || c > 'Z')
        return;
    const uint8_t* char_bitmap = font[c - ' '];
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            bool pixel_on = (char_bitmap[i] >> j) & 0x01;
            pixel(x + i, y + j, pixel_on);
        }
    }
}


void draw_string(int x, int y, const char* str)
{
    while (*str)
    {
        draw_char(x, y, *str);
        x += 6;
        str++;
    }
}


void draw_number(int x, int y, int value)
{
    char val[8];
    snprintf(val, sizeof(val), "%3d%%", value);
    draw_string(x, y, val);
}