#include "lib/int.h"

#define __divide_by_zero() \
    do {                        \
        int a = 1;              \
        a /= 2;                 \
        int b = 1 / a;          \
        b++;                    \
    } while(0)

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if(!d) __divide_by_zero();

    uint64_t q = 0, bit = 1;

    while (((int64_t) d) >= 0) {
        d <<= 1;
        bit <<= 1;
    }

    while (bit) {
        if (d <= n) {
            n -= d;
            q += bit;
        }

        d >>= 1;
        bit >>= 1;
    }

    return q;
}
