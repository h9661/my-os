// filepath: /Users/chan-uhyeon/Programming/my-os/kernel/src/vga/vga.c
#include "../../include/vga/vga.h"

/* Input byte from port */
uint8_t vga_inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* Output byte to port */
void vga_outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Initialize VGA text mode */
void vga_initialize(void) {
    /* Disable VGA cursor blinking to avoid display issues */
    vga_outb(0x3D4, 0x0A);
    uint8_t cursor_start = vga_inb(0x3D5);
    vga_outb(0x3D5, cursor_start & 0xDF);  /* Clear bit 5 to disable blinking */
    
    /* Set cursor shape (underline cursor) */
    vga_outb(0x3D4, 0x0A);
    vga_outb(0x3D5, 14);                   /* Cursor start line */
    vga_outb(0x3D4, 0x0B);
    vga_outb(0x3D5, 15);                   /* Cursor end line */
    
    /* Clear any potential artifacts in VGA buffer */
    uint16_t* vga_buffer = (uint16_t*)VGA_BUFFER_ADDR;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
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
