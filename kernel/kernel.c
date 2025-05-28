/* kernel.c - Simple kernel that displays "Hello, World!" */

/* Define size_t type */
typedef unsigned int size_t;

/* VGA text mode color constants */
enum vga_color {
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
};

/* Create a VGA color byte from foreground and background colors */
static inline unsigned char vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

/* Create a VGA character entry (character + color) */
static inline unsigned short vga_entry(unsigned char c, unsigned char color) {
    return (unsigned short) c | (unsigned short) color << 8;
}

/* String length function */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

/* VGA buffer dimensions */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

/* Terminal state variables */
size_t terminal_row;
size_t terminal_column;
unsigned char terminal_color;
unsigned short* terminal_buffer;

/* Initialize the terminal */
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_buffer = (unsigned short*) 0xB8000; /* VGA text buffer address */
    
    /* Clear the screen */
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

/* Set terminal color */
void terminal_setcolor(unsigned char color) {
    terminal_color = color;
}

/* Put a character at specific position */
void terminal_putentryat(char c, unsigned char color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

/* Handle newline */
void terminal_newline(void) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
        terminal_row = 0;
}

/* Put a character with the current color at the current position */
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

/* Convert integer to string */
void int_to_string(int value, char* str) {
    int i = 0;
    int is_negative = 0;
    
    /* Handle 0 explicitly */
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    /* Handle negative numbers */
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    /* Process individual digits */
    while (value != 0) {
        int remainder = value % 10;
        str[i++] = remainder + '0';
        value = value / 10;
    }
    
    /* Add negative sign if needed */
    if (is_negative)
        str[i++] = '-';
    
    str[i] = '\0'; /* Null-terminate string */
    
    /* Reverse the string */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/* Get CPU information */
void get_cpu_info(char* cpu_vendor) {
    unsigned int eax, ebx, ecx, edx;
    
    /* Execute CPUID instruction with EAX=0 */
    __asm__ volatile("cpuid"
                    : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
                    : "0" (0));
    
    /* Copy vendor ID string (12 bytes in EBX, EDX, ECX) */
    ((unsigned int*)cpu_vendor)[0] = ebx;
    ((unsigned int*)cpu_vendor)[1] = edx;
    ((unsigned int*)cpu_vendor)[2] = ecx;
    cpu_vendor[12] = '\0'; /* Null-terminate string */
}

/* Get memory (RAM) size - a very simple estimation */
unsigned int get_memory_size(void) {
    /* Simplified RAM detection - just return 640KB for basic memory */
    /* In a real OS, you would use methods like INT 15h or parsing memory maps */
    return 640; /* Return 640KB (conventional memory in real mode) */
}

/* Write a string to the terminal */
void terminal_writestring(const char* data) {
    size_t len = strlen(data);
    for (size_t i = 0; i < len; i++)
        terminal_putchar(data[i]);
}

/* Kernel main function */
void kernel_main(void) {
    /* Initialize terminal interface */
    terminal_initialize();

    /* Print "Hello, World!" */
    terminal_writestring("Hello, World!\n");
    terminal_writestring("Welcome to my simple OS!\n");
    terminal_writestring("\n");

    /* CPU and memory information */
    char cpu_vendor[13];
    get_cpu_info(cpu_vendor);
    terminal_writestring("=== Hardware Information ===\n");
    terminal_writestring("CPU Vendor: ");
    terminal_writestring(cpu_vendor);
    terminal_putchar('\n');

    unsigned int memory_size = get_memory_size();
    terminal_writestring("Memory Size: ");
    
    char memory_size_str[10];
    int_to_string(memory_size, memory_size_str);
    terminal_writestring(memory_size_str);
    terminal_writestring(" KB\n");
    terminal_writestring("==========================\n");
}