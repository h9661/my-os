#ifndef TYPES_H
#define TYPES_H

/* Basic type definitions */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned int size_t;

/* Simple boolean type for kernel use */
#define bool int
#define true 1
#define false 0

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* TYPES_H */
