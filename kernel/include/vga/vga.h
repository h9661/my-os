#ifndef VGA_H
#define VGA_H

#include "../common/types.h"

/* VGA text mode constants */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_BUFFER_ADDR 0xB8000

/* VGA cursor control ports */
#define VGA_CURSOR_CTRL_PORT 0x3D4
#define VGA_CURSOR_DATA_PORT 0x3D5

/* VGA color constants */
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

/* VGA utility functions */
uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg);
uint16_t vga_entry(unsigned char c, uint8_t color);

/* Low-level output functions */
void vga_outb(uint16_t port, uint8_t value);
void vga_set_cursor_position(uint16_t position);

#endif /* VGA_H */
