#include <stdint.h>

uint64_t uptime();

void play(uint32_t freq);
void stop();
void beep();

void sleep(uint32_t milis);

void pit_init();
