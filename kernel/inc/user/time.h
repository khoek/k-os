#ifndef KERNEL_USER_TIME_H
#define KERNEL_USER_TIME_H

#include "common/types.h"

typedef uint64_t time_t;
typedef uint32_t suseconds_t;

struct timeval {
		time_t tv_sec;
		suseconds_t	tv_usec;
};

#endif
