#ifndef MINI_OS_STDINT_H
#define MINI_OS_STDINT_H

// Minimal freestanding stdint.h for our kernel build.
// We compile with -nostdinc, so we provide the fixed-width types ourselves.

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int64_t;
typedef unsigned long long uint64_t;

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

#define UINT8_MAX  ((uint8_t)0xFF)
#define UINT16_MAX ((uint16_t)0xFFFF)
#define UINT32_MAX ((uint32_t)0xFFFFFFFFu)
#define UINT64_MAX ((uint64_t)0xFFFFFFFFFFFFFFFFull)

#endif