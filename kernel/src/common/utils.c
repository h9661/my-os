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

/* Convert integer to hexadecimal string */
void int_to_hex(int value, char* str) {
    const char hex_chars[] = "0123456789ABCDEF";
    int i = 0;
    
    /* Handle 0 explicitly */
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    /* Convert to unsigned for proper hex handling */
    unsigned int uvalue = (unsigned int)value;
    
    /* Process individual hex digits */
    while (uvalue != 0) {
        int remainder = uvalue % 16;
        str[i++] = hex_chars[remainder];
        uvalue = uvalue / 16;
    }
    
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

/* String copy function */
void strcpy(char* dest, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* String concatenation function */
void strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

/* String copy with length limit */
void strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
}

/* String comparison with length limit */
int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        if (str1[i] == '\0') {
            break;
        }
    }
    return 0;
}

/* Convert string to integer */
int string_to_int(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    
    /* Handle negative sign */
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    /* Convert digits */
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}
