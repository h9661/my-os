#include "../../include/terminal/terminal.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* Terminal state */
terminal_t terminal;

/* Static scroll buffer for storing terminal history */
static uint16_t scroll_buffer[TERMINAL_BUFFER_LINES * VGA_WIDTH];

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
    terminal.scroll_buffer = scroll_buffer;
    terminal.total_lines = 0;
    terminal.current_line = VGA_HEIGHT - 1;  /* Start with cursor at bottom of first screen */
    terminal.scroll_offset = 0;
    
    /* Initialize scroll buffer with blank entries */
    for (size_t i = 0; i < TERMINAL_BUFFER_LINES * VGA_WIDTH; i++) {
        terminal.scroll_buffer[i] = vga_entry(' ', terminal.color);
    }
    
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
    /* Clear both VGA buffer and scroll buffer */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal.buffer[index] = vga_entry(' ', terminal.color);
        }
    }
    
    /* Clear scroll buffer */
    for (size_t i = 0; i < TERMINAL_BUFFER_LINES * VGA_WIDTH; i++) {
        terminal.scroll_buffer[i] = vga_entry(' ', terminal.color);
    }
    
    terminal.row = 0;
    terminal.column = 0;
    terminal.total_lines = 0;
    terminal.current_line = VGA_HEIGHT - 1;  /* Cursor starts at bottom of first screen */
    terminal.scroll_offset = 0;
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
    
    /* Ensure character is printable ASCII (0x20-0x7E) or space */
    unsigned char safe_char = (unsigned char)c;
    if (safe_char < 0x20 || safe_char > 0x7E) {
        safe_char = '?'; /* Replace non-printable with question mark */
    }
    
    uint16_t entry = vga_entry(safe_char, color);
    
    /* Write to VGA buffer for immediate display */
    const size_t vga_index = y * VGA_WIDTH + x;
    if (vga_index < VGA_WIDTH * VGA_HEIGHT) {
        terminal.buffer[vga_index] = entry;
    }
    
    /* Also write to scroll buffer at the current location */
    /* For the current screen, we store it at current_line - (VGA_HEIGHT - 1) + y */
    if (terminal.total_lines >= VGA_HEIGHT) {
        size_t scroll_line = terminal.total_lines - VGA_HEIGHT + y;
        if (scroll_line < TERMINAL_BUFFER_LINES) {
            const size_t scroll_index = scroll_line * VGA_WIDTH + x;
            if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                terminal.scroll_buffer[scroll_index] = entry;
            }
        }
    } else {
        /* In the initial screen, just store at y */
        if (y < TERMINAL_BUFFER_LINES) {
            const size_t scroll_index = y * VGA_WIDTH + x;
            if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                terminal.scroll_buffer[scroll_index] = entry;
            }
        }
    }
}

/* Sync current VGA content to scroll buffer */
void terminal_sync_to_scroll_buffer(void) {
    /* Calculate the starting line in scroll buffer for current screen */
    size_t base_line = 0;
    if (terminal.total_lines >= VGA_HEIGHT) {
        base_line = terminal.total_lines - VGA_HEIGHT;
    }
    
    /* Copy all VGA content to corresponding scroll buffer lines */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t vga_index = y * VGA_WIDTH + x;
            size_t scroll_line = base_line + y;
            
            if (scroll_line < TERMINAL_BUFFER_LINES) {
                size_t scroll_index = scroll_line * VGA_WIDTH + x;
                if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                    terminal.scroll_buffer[scroll_index] = terminal.buffer[vga_index];
                }
            }
        }
    }
}

/* Scroll the terminal up by one line (when content exceeds screen) */
void terminal_scroll(void) {
    /* First, save the current VGA screen content to scroll buffer before scrolling */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t vga_index = y * VGA_WIDTH + x;
            size_t scroll_line = terminal.current_line - terminal.row + y;
            
            if (scroll_line < TERMINAL_BUFFER_LINES) {
                size_t scroll_index = scroll_line * VGA_WIDTH + x;
                if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                    terminal.scroll_buffer[scroll_index] = terminal.buffer[vga_index];
                }
            }
        }
    }
    
    /* Advance to next line in scroll buffer */
    terminal.current_line++;
    
    /* Handle scroll buffer wraparound */
    if (terminal.current_line >= TERMINAL_BUFFER_LINES) {
        /* Shift scroll buffer content up */
        for (size_t y = 0; y < TERMINAL_BUFFER_LINES - 1; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t current_index = y * VGA_WIDTH + x;
                const size_t next_index = (y + 1) * VGA_WIDTH + x;
                terminal.scroll_buffer[current_index] = terminal.scroll_buffer[next_index];
            }
        }
        
        /* Clear the last line in scroll buffer */
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = (TERMINAL_BUFFER_LINES - 1) * VGA_WIDTH + x;
            terminal.scroll_buffer[index] = vga_entry(' ', terminal.color);
        }
        
        terminal.current_line = TERMINAL_BUFFER_LINES - 1;
    }
    
    /* Scroll the VGA buffer up */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current_index = y * VGA_WIDTH + x;
            const size_t next_index = (y + 1) * VGA_WIDTH + x;
            terminal.buffer[current_index] = terminal.buffer[next_index];
        }
    }
    
    /* Clear the last line in VGA buffer */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal.buffer[index] = vga_entry(' ', terminal.color);
    }
}

/* Refresh the display with current scroll buffer content */
void terminal_refresh_display(void) {
    /* Calculate which lines from scroll buffer to show */
    /* We want to show VGA_HEIGHT lines ending at (total_lines - scroll_offset - 1) */
    
    if (terminal.total_lines < VGA_HEIGHT) {
        /* Not enough content to scroll, just show what we have */
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                size_t vga_index = y * VGA_WIDTH + x;
                
                if (y < terminal.total_lines) {
                    size_t scroll_index = y * VGA_WIDTH + x;
                    if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                        terminal.buffer[vga_index] = terminal.scroll_buffer[scroll_index];
                    } else {
                        terminal.buffer[vga_index] = vga_entry(' ', terminal.color);
                    }
                } else {
                    terminal.buffer[vga_index] = vga_entry(' ', terminal.color);
                }
            }
        }
        return;
    }
    
    /* Calculate the last line to show (0-based) */
    size_t end_line = terminal.total_lines - 1 - terminal.scroll_offset;
    
    /* Calculate start line (VGA_HEIGHT-1 lines before end_line) */
    size_t start_line = 0;
    if (end_line >= VGA_HEIGHT - 1) {
        start_line = end_line - (VGA_HEIGHT - 1);
    }
    
    /* Copy lines from scroll buffer to VGA buffer */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        size_t source_line = start_line + y;
        
        if (source_line <= end_line && source_line < TERMINAL_BUFFER_LINES) {
            /* Copy this line from scroll buffer */
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                size_t scroll_index = source_line * VGA_WIDTH + x;
                size_t vga_index = y * VGA_WIDTH + x;
                
                if (scroll_index < TERMINAL_BUFFER_LINES * VGA_WIDTH) {
                    terminal.buffer[vga_index] = terminal.scroll_buffer[scroll_index];
                } else {
                    terminal.buffer[vga_index] = vga_entry(' ', terminal.color);
                }
            }
        } else {
            /* Clear this line if no content available */
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                size_t vga_index = y * VGA_WIDTH + x;
                terminal.buffer[vga_index] = vga_entry(' ', terminal.color);
            }
        }
    }
    
    /* Update cursor position relative to the displayed content */
    if (terminal.scroll_offset == 0) {
        /* Normal case - cursor should be at its normal position */
        terminal_update_cursor();
    } else {
        /* When scrolled, hide cursor or adjust its position */
        /* For now, just hide cursor by setting it off-screen */
        vga_set_cursor_position(VGA_WIDTH * VGA_HEIGHT);
    }
}

/* Scroll up by specified number of lines */
void terminal_scroll_up(size_t lines) {
    /* Before first scroll, ensure current VGA content is in scroll buffer */
    if (terminal.scroll_offset == 0) {
        terminal_sync_to_scroll_buffer();
    }
    
    if (terminal.total_lines < VGA_HEIGHT) {
        /* Not enough content to scroll */
        return;
    }
    
    size_t max_scroll = terminal.total_lines - VGA_HEIGHT;
    
    if (terminal.scroll_offset + lines <= max_scroll) {
        terminal.scroll_offset += lines;
    } else {
        terminal.scroll_offset = max_scroll;
    }
    
    terminal_refresh_display();
}

/* Scroll down by specified number of lines */
void terminal_scroll_down(size_t lines) {
    if (terminal.scroll_offset >= lines) {
        terminal.scroll_offset -= lines;
    } else {
        terminal.scroll_offset = 0;
    }
    
    terminal_refresh_display();
}

/* Scroll to bottom of terminal */
void terminal_scroll_to_bottom(void) {
    terminal.scroll_offset = 0;
    terminal_refresh_display();
}

/* Page up (scroll up by full screen) */
void terminal_page_up(void) {
    terminal_scroll_up(VGA_HEIGHT);
}

/* Page down (scroll down by full screen) */
void terminal_page_down(void) {
    terminal_scroll_down(VGA_HEIGHT);
}

/* Handle newline */
void terminal_newline(void) {
    terminal.column = 0;
    if (++terminal.row == VGA_HEIGHT) {
        terminal_scroll();
        terminal.row = VGA_HEIGHT - 1;
        
        /* Auto-scroll to bottom when new content is added */
        terminal.scroll_offset = 0;
    }
    
    /* Increment total_lines counter when moving to a new line */
    terminal.total_lines++;
    
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
