#ifndef TERMINAL_H
#define TERMINAL_H

#include "../common/types.h"
#include "../vga/vga.h"

/* Terminal configuration */
#define TERMINAL_BUFFER_LINES 1000  /* Total lines in scroll buffer */
#define TERMINAL_VISIBLE_LINES VGA_HEIGHT  /* Lines visible on screen */

/* Terminal structure */
typedef struct {
    size_t row;           /* Current row position */
    size_t column;        /* Current column position */
    uint8_t color;        /* Current text color */
    uint16_t* buffer;     /* VGA display buffer */
    uint16_t* scroll_buffer;  /* Extended scroll buffer */
    size_t total_lines;   /* Total lines written to buffer */
    size_t current_line;  /* Current line in scroll buffer */
    size_t scroll_offset; /* Current scroll position */
} terminal_t;

/* Global terminal state - declared in terminal.c */
extern terminal_t terminal;

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
void terminal_scroll_up(size_t lines);
void terminal_scroll_down(size_t lines);
void terminal_scroll_to_bottom(void);
void terminal_page_up(void);
void terminal_page_down(void);
void terminal_refresh_display(void);
void terminal_update_cursor(void);

void terminal_backspace(void);

/* Terminal formatting functions */
void terminal_print_header(const char* title);
void terminal_print_separator(void);

/* Terminal diagnostic functions */
void terminal_test_vga_buffer(void);

#endif /* TERMINAL_H */
