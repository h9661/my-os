#include "../../include/keyboard/keyboard.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* US QWERTY scancode to ASCII conversion table */
static const char scancode_ascii_table[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', /* 0x00-0x09 */
    '9', '0', '-', '=', '\b','\t','q', 'w', 'e', 'r', /* 0x0A-0x13 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, /* 0x14-0x1D */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 0x1E-0x27 */
    '\'','`',  0, '\\','z', 'x', 'c', 'v', 'b', 'n', /* 0x28-0x31 */
    'm', ',', '.', '/',  0, '*',  0, ' ',  0,  0,     /* 0x32-0x3B */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x3C-0x45 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x46-0x4F */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x50-0x59 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x5A-0x63 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x64-0x6D */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x6E-0x77 */
    0,   0,   0,   0,   0,   0,   0,   0             /* 0x78-0x7F */
};

/* Shifted scancode to ASCII conversion table */
static const char scancode_ascii_shift_table[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', /* 0x00-0x09 */
    '(', ')', '_', '+', '\b','\t','Q', 'W', 'E', 'R', /* 0x0A-0x13 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, /* 0x14-0x1D */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 0x1E-0x27 */
    '"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', /* 0x28-0x31 */
    'M', '<', '>', '?',  0, '*',  0, ' ',  0,  0,     /* 0x32-0x3B */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x3C-0x45 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x46-0x4F */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x50-0x59 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x5A-0x63 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x64-0x6D */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   /* 0x6E-0x77 */
    0,   0,   0,   0,   0,   0,   0,   0             /* 0x78-0x7F */
};

/* Input buffer */
#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static size_t input_buffer_head = 0;
static size_t input_buffer_tail = 0;
static size_t input_buffer_count = 0;

/* Keyboard state */
static keyboard_state_t keyboard_state = {0};

/* Port I/O helper functions */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Initialize input buffer */
void input_buffer_init(void) {
    input_buffer_head = 0;
    input_buffer_tail = 0;
    input_buffer_count = 0;
}

/* Add character to input buffer */
void input_buffer_add_char(char c) {
    if (input_buffer_count < INPUT_BUFFER_SIZE) {
        input_buffer[input_buffer_head] = c;
        input_buffer_head = (input_buffer_head + 1) % INPUT_BUFFER_SIZE;
        input_buffer_count++;
    }
}

/* Get character from input buffer */
char input_buffer_get_char(void) {
    if (input_buffer_count > 0) {
        char c = input_buffer[input_buffer_tail];
        input_buffer_tail = (input_buffer_tail + 1) % INPUT_BUFFER_SIZE;
        input_buffer_count--;
        return c;
    }
    return 0;
}

/* Check if input buffer has data */
bool input_buffer_has_data(void) {
    return input_buffer_count > 0;
}

/* Clear input buffer */
void input_buffer_clear(void) {
    input_buffer_head = 0;
    input_buffer_tail = 0;
    input_buffer_count = 0;
}

/* Convert scancode to ASCII */
char scancode_to_ascii(uint8_t scancode, bool shift) {
    if (scancode >= 128) {
        return 0; /* Extended or key release */
    }
    
    if (shift || keyboard_state.caps_lock) {
        return scancode_ascii_shift_table[scancode];
    } else {
        return scancode_ascii_table[scancode];
    }
}

/* Process keyboard input */
void keyboard_process_input(uint8_t scancode) {
    /* Handle key releases (scancode >= 0x80) */
    if (scancode >= 0x80) {
        scancode -= 0x80; /* Convert to make code */
        
        /* Handle modifier key releases */
        switch (scancode) {
            case KEY_SHIFT_L:
            case KEY_SHIFT_R:
                keyboard_state.shift_pressed = false;
                break;
            case KEY_CTRL:
                keyboard_state.ctrl_pressed = false;
                break;
            case KEY_ALT:
                keyboard_state.alt_pressed = false;
                break;
        }
        return;
    }
    
    /* Handle key presses */
    switch (scancode) {
        case KEY_SHIFT_L:
        case KEY_SHIFT_R:
            keyboard_state.shift_pressed = true;
            break;
            
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = true;
            break;
            
        case KEY_ALT:
            keyboard_state.alt_pressed = true;
            break;
            
        case KEY_CAPS_LOCK:
            keyboard_state.caps_lock = !keyboard_state.caps_lock;
            break;
            
        default: {
            /* Convert scancode to ASCII and display */
            char ascii = scancode_to_ascii(scancode, keyboard_state.shift_pressed);
            if (ascii != 0) {
                /* Add to input buffer */
                input_buffer_add_char(ascii);
            }
            break;
        }
    }
}

/* Keyboard interrupt handler */
void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    keyboard_process_input(scancode);
}

/* Initialize keyboard */
void keyboard_initialize(void) {
    /* Initialize input buffer */
    input_buffer_init();
    
    /* Clear keyboard state */
    keyboard_state.shift_pressed = false;
    keyboard_state.ctrl_pressed = false;
    keyboard_state.alt_pressed = false;
    keyboard_state.caps_lock = false;
    
    /* Clear keyboard buffer by reading any pending data */
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        inb(KEYBOARD_DATA_PORT);
    }
    
    terminal_writeline("Keyboard initialized successfully!");
}
