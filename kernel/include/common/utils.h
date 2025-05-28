#ifndef UTILS_H
#define UTILS_H

#include "types.h"

/* String utility functions */
size_t strlen(const char* str);
void int_to_string(int value, char* str);
int strcmp(const char* str1, const char* str2);
void strcpy(char* dest, const char* src);

/* Memory utility functions */
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

#endif /* UTILS_H */
