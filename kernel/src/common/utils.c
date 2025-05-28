#include "../../include/common/utils.h"

/* String length function */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
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

/* Compare two strings */
int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

/* Copy string */
void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++))
        ;
}

/* Set memory to specific value */
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

/* Copy memory */
void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

/* Compare memory */
int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}
