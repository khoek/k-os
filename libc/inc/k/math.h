#ifndef LIBC_K_MATH_H
#define LIBC_K_MATH_H

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define DIV_DOWN(x, y) ((x) / (y))
#define DIV_UP(x, y)   (((x) == 0) ? 0 : ((((x) - 1) / (y)) + 1))

#endif
