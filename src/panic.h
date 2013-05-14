#include "common.h"

void die() NORETURN;

void panic(char* message) NORETURN;
void panicf(char* fmt, ...) NORETURN;
