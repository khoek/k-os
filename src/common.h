#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#define ALIGN(a)    __attribute__((aligned((a))))
#define UNUSED(v) v __attribute__((unused))
#define NORETURN    __attribute__((noreturn))
#define PACKED      __attribute__((packed))

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define DIV_DOWN(x, y) ((x) / (y))
#define DIV_UP(x, y)   ((((x) - 1) / (y)) + 1)

#define BIT_SET(bits, bit) (((uint32_t) bits) & ((uint32_t) bit))

#endif
