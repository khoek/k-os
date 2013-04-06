#include "rand.h"

uint32_t holdrand;

void srand(uint32_t seed) {
   holdrand = seed;
}

uint8_t rand() {
   return (uint8_t) (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}
