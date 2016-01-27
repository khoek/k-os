#ifndef KERNEL_LIB_INT_H
#define KERNEL_LIB_INT_H

#define INT16_MIN  (-32768)
#define INT16_MAX  32767

#define INT32_MIN  (-2147483648)
#define INT32_MAX  2147483647

#define UINT16_MIN 0
#define UINT16_MAX 65535

#define UINT32_MIN 0
#define UINT32_MAX 4294967295

#define SSIZE_MIN  INT32_MIN
#define SSIZE_MAX  INT32_MAX

#define SIZE_MIN   UINT32_MIN
#define SIZE_MAX   UINT32_MAX

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef int32_t ssize_t;
typedef uint32_t off_t;

#endif
