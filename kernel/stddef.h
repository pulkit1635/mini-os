#ifndef MINI_OS_STDDEF_H
#define MINI_OS_STDDEF_H

// Minimal freestanding stddef.h for our kernel build.

typedef __SIZE_TYPE__ size_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif