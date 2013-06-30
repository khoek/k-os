#include "init.h"
#include "rand.h"
#include "tsc.h"
#include "log.h"

static uint32_t holdrand;

void srand(uint32_t seed) {
    holdrand = seed;
}

uint8_t rand() {
    return (uint8_t) (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

static INITCALL rand_init() {
    srand((uint32_t) rdtsc());

    logf("rand - seeded unsecure pseudorandom number generator");

    return 0;
}

core_initcall(rand_init);
