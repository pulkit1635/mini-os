#ifndef MINI_OS_STDBOOL_H
#define MINI_OS_STDBOOL_H

// Minimal freestanding stdbool.h for our kernel build.

#ifndef __cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif

#endif