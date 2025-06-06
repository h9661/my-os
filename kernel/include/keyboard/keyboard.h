#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../common/types.h"

/* Keyboard ports */
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64

/* Special key codes */
#define KEY_ESCAPE     0x01
#define KEY_BACKSPACE  0x0E
#define KEY_TAB        0x0F
#define KEY_ENTER      0x1C
#define KEY_CTRL       0x1D
#define KEY_SHIFT_L    0x2A
#define KEY_SHIFT_R    0x36
#define KEY_ALT        0x38
#define KEY_SPACE      0x39
#define KEY_CAPS_LOCK  0x3A

/* Function key codes */
#define KEY_F1         0x3B
#define KEY_F2         0x3C
#define KEY_F3         0x3D
#define KEY_F4         0x3E
#define KEY_F5         0x3F
#define KEY_F6         0x40
#define KEY_F7         0x41
#define KEY_F8         0x42
#define KEY_F9         0x43
#define KEY_F10        0x44

/* Arrow key codes */
#define KEY_UP         0x48
#define KEY_DOWN       0x50
#define KEY_LEFT       0x4B
#define KEY_RIGHT      0x4D

/* Page navigation codes */
#define KEY_PAGE_UP    0x49
#define KEY_PAGE_DOWN  0x51
#define KEY_HOME       0x47
#define KEY_END        0x4F

/* Modifier states */
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
} keyboard_state_t;

/* Function prototypes */
void keyboard_initialize(void);
void keyboard_handler(void);
char scancode_to_ascii(uint8_t scancode, bool shift);
void keyboard_process_input(uint8_t scancode);

/* Input buffer functions */
void input_buffer_init(void);
void input_buffer_add_char(char c);
char input_buffer_get_char(void);
bool input_buffer_has_data(void);
void input_buffer_clear(void);

#endif /* KEYBOARD_H */
