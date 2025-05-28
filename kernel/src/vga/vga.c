#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* VGA cursor control ports */
#define VGA_CURSOR_CTRL_PORT 0x3D4
#define VGA_CURSOR_DATA_PORT 0x3D5

/* Terminal state */
static terminal_t terminal;

/* Output byte to port */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Update hardware cursor position */
void terminal_update_cursor(void) {
    uint16_t position = terminal.row * VGA_WIDTH + terminal.column;
    
    /* Set cursor position low byte */
    outb(VGA_CURSOR_CTRL_PORT, 0x0F);
    outb(VGA_CURSOR_DATA_PORT, (uint8_t)(position & 0xFF));
    
    /* Set cursor position high byte */
    outb(VGA_CURSOR_CTRL_PORT, 0x0E);
    outb(VGA_CURSOR_DATA_PORT, (uint8_t)((position >> 8) & 0xFF));
}

/* Create a VGA color byte from foreground and background colors */
uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

/* Create a VGA character entry (character + color) */
uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

/* Initialize the terminal */
void terminal_initialize(void) {
    terminal.row = 0;
    terminal.column = 0;
    terminal.color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal.buffer = (uint16_t*) VGA_BUFFER_ADDR;
    
    terminal_clear();
    terminal_update_cursor();
}

/* Clear the screen */
void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal.buffer[index] = vga_entry(' ', terminal.color);
        }
    }
    terminal.row = 0;
    terminal.column = 0;
    terminal_update_cursor();
}

/* Set terminal color */
void terminal_setcolor(uint8_t color) {
    terminal.color = color;
}

/* Put a character at specific position */
void terminal_putchar_at(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal.buffer[index] = vga_entry(c, color);
}

/* Scroll the terminal up by one line */
void terminal_scroll(void) {
    /* Move all lines up by one */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current_index = y * VGA_WIDTH + x;
            const size_t next_index = (y + 1) * VGA_WIDTH + x;
            terminal.buffer[current_index] = terminal.buffer[next_index];
        }
    }
    
    /* Clear the last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal.buffer[index] = vga_entry(' ', terminal.color);
    }
}

/* Handle newline */
void terminal_newline(void) {
    terminal.column = 0;
    if (++terminal.row == VGA_HEIGHT) {
        terminal_scroll();
        terminal.row = VGA_HEIGHT - 1;
    }
    terminal_update_cursor();
}

/* Put a character with the current color at the current position */
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    
    terminal_putchar_at(c, terminal.color, terminal.column, terminal.row);
    if (++terminal.column == VGA_WIDTH) {
        terminal_newline();
    } else {
        terminal_update_cursor();
    }
}

/* Write a string to the terminal */
void terminal_writestring(const char* data) {
    size_t len = strlen(data);
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(data[i]);
    }
}

/* Write a string with newline */
void terminal_writeline(const char* data) {
    terminal_writestring(data);
    terminal_putchar('\n');
}

/* Print a formatted header */
void terminal_print_header(const char* title) {
    uint8_t old_color = terminal.color;
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("=== ");
    terminal_writestring(title);
    terminal_writestring(" ===\n");
    
    terminal_setcolor(old_color);
}

/* Print a separator line */
void terminal_print_separator(void) {
    uint8_t old_color = terminal.color;
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
    for (int i = 0; i < 40; i++) {
        terminal_putchar('=');
    }
    terminal_putchar('\n');
    
    terminal_setcolor(old_color);
}
