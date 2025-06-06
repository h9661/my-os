#include "../../include/terminal/terminal.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* Terminal state */
static terminal_t terminal;

/* Update hardware cursor position */
void terminal_update_cursor(void) {
    uint16_t position = terminal.row * VGA_WIDTH + terminal.column;
    vga_set_cursor_position(position);
}

/* Initialize the terminal */
void terminal_initialize(void) {
    /* Initialize VGA hardware first */
    vga_initialize();
    
    /* Initialize terminal state */
    terminal.row = 0;
    terminal.column = 0;
    terminal.color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal.buffer = (uint16_t*) VGA_BUFFER_ADDR;
    
    /* Verify VGA buffer is accessible */
    volatile uint16_t* test_ptr = (volatile uint16_t*)VGA_BUFFER_ADDR;
    uint16_t test_value = vga_entry('T', terminal.color);
    *test_ptr = test_value;
    
    /* Clear the screen */
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
    /* Bounds checking to prevent buffer overflow */
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return;
    }
    
    const size_t index = y * VGA_WIDTH + x;
    
    /* Additional safety check for buffer bounds */
    if (index >= VGA_WIDTH * VGA_HEIGHT) {
        return;
    }
    
    /* Ensure character is printable ASCII (0x20-0x7E) or space */
    unsigned char safe_char = (unsigned char)c;
    if (safe_char < 0x20 || safe_char > 0x7E) {
        safe_char = '?'; /* Replace non-printable with question mark */
    }
    
    terminal.buffer[index] = vga_entry(safe_char, color);
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

/* Handle backspace */
void terminal_backspace(void) {
    if (terminal.column > 0) {
        terminal.column--;
        terminal_putchar_at(' ', terminal.color, terminal.column, terminal.row);
        terminal_update_cursor();
    } else if (terminal.row > 0) {
        terminal.row--;
        terminal.column = VGA_WIDTH - 1;
        /* Find the last non-space character in the previous line */
        while (terminal.column > 0) {
            const size_t index = terminal.row * VGA_WIDTH + terminal.column - 1;
            if ((terminal.buffer[index] & 0xFF) != ' ') {
                break;
            }
            terminal.column--;
        }
        terminal_update_cursor();
    }
}

/* Put a character with the current color at the current position */
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    } else if (c == '\b') {
        terminal_backspace();
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

/* Test VGA buffer integrity */
void terminal_test_vga_buffer(void) {
    uint16_t test_pattern = vga_entry('X', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    
    /* Test writing to corners of screen */
    terminal.buffer[0] = test_pattern;                                    /* Top-left */
    terminal.buffer[VGA_WIDTH - 1] = test_pattern;                       /* Top-right */
    terminal.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH] = test_pattern;        /* Bottom-left */
    terminal.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + VGA_WIDTH - 1] = test_pattern; /* Bottom-right */
    
    /* Small delay to see the pattern */
    for (volatile int i = 0; i < 1000000; i++);
    
    /* Clear test pattern */
    terminal.buffer[0] = vga_entry(' ', terminal.color);
    terminal.buffer[VGA_WIDTH - 1] = vga_entry(' ', terminal.color);
    terminal.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH] = vga_entry(' ', terminal.color);
    terminal.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + VGA_WIDTH - 1] = vga_entry(' ', terminal.color);
}
