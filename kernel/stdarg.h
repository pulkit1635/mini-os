#ifndef MINI_OS_STDARG_H
#define MINI_OS_STDARG_H

// Minimal freestanding stdarg.h for our kernel build.
// GCC/Clang provide the underlying builtins.

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)

#define va_copy(dest, src) __builtin_va_copy(dest, src)

#endif