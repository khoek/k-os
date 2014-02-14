#include "lib/int.h"

#define __divide_by_zero() \
    do {                        \
        int a = 1;              \
        a /= 2;                 \
        int b = 1 / a;          \
        b++;                    \
    } while(0)

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if(n < d) return 0;

    if(!d) __divide_by_zero();

    uint64_t times = 0;
    while(n > d) {
        n -= d;
        times++;
    }

    return times;
}
