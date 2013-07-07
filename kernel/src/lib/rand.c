#include "init.h"
#include "rand.h"
#include "tsc.h"
#include "log.h"

static uint32_t holdrand;

void srand(uint32_t seed) {
    holdrand = seed;
}

uint8_t rand8() {
    return (uint8_t) (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

uint16_t rand16() {
    return (((uint16_t) rand8()) << 8)
        | ((uint16_t) rand8());
}

uint32_t rand32() {
    return (((uint32_t) rand16()) << 16)
        | ((uint32_t) rand16());
}

static INITCALL rand_init() {
    srand((uint32_t) rdtsc());

    logf("rand - seeded unsecure pseudorandom number generator");

    return 0;
}

core_initcall(rand_init);
