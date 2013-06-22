#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#define STR(s)  #s
#define XSTR(s) STR(s)

#define ALIGN(a)    __attribute__((aligned((a))))
#define UNUSED(v) v __attribute__((unused))
#define NORETURN    __attribute__((noreturn))
#define PACKED      __attribute__((packed))

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define DIV_DOWN(x, y) ((x) / (y))
#define DIV_UP(x, y)   ((((x) - 1) / (y)) + 1)

#define BIT_SET(flags, mask) (((uint32_t) flags) & ((uint32_t) mask))

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &(((type *) 0)->member))
#endif

#define containerof(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type, member) );})

#include "debug.h"

#endif
