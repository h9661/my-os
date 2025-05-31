#ifndef TERMINAL_H
#define TERMINAL_H

#include "../common/types.h"
#include "../vga/vga.h"

/* Terminal structure */
typedef struct {
    size_t row;
    size_t column;
    uint8_t color;
    uint16_t* buffer;
} terminal_t;

/* Terminal functions */
void terminal_initialize(void);
void terminal_clear(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_putchar_at(char c, uint8_t color, size_t x, size_t y);
void terminal_writestring(const char* data);
void terminal_writeline(const char* data);
void terminal_newline(void);
void terminal_scroll(void);
void terminal_update_cursor(void);
void terminal_backspace(void);

/* Terminal formatting functions */
void terminal_print_header(const char* title);
void terminal_print_separator(void);

#endif /* TERMINAL_H */
