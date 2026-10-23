#ifndef OLED_H
#define OLED_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
void I2C_init(void);
void oled_init(void);
void pixel(int x, int y, bool on);
void clear_framebuffer(void);
void display_framebeffer(void);
void draw_line(int x0, int y0, int x1, int y1);
void draw_rectangle(int x0, int y0, int x1, int y1);
void draw_char(int x, int y, char c);
void draw_string(int x, int y, const char *str);
void draw_number(int x, int y, int value);
#endif