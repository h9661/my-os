// filepath: /Users/chan-uhyeon/Programming/my-os/kernel/src/vga/vga.c
#include "../../include/vga/vga.h"

/* Output byte to port */
void vga_outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Set the VGA hardware cursor position */
void vga_set_cursor_position(uint16_t position) {
    /* Set cursor position low byte */
    vga_outb(VGA_CURSOR_CTRL_PORT, 0x0F);
    vga_outb(VGA_CURSOR_DATA_PORT, (uint8_t)(position & 0xFF));
    
    /* Set cursor position high byte */
    vga_outb(VGA_CURSOR_CTRL_PORT, 0x0E);
    vga_outb(VGA_CURSOR_DATA_PORT, (uint8_t)((position >> 8) & 0xFF));
}

/* Create a VGA color byte from foreground and background colors */
uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

/* Create a VGA character entry (character + color) */
uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}
