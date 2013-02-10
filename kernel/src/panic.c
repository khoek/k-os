#include <stdbool.h>
#include <stdarg.h>
#include <printf.h>
#include "panic.h"
#include "idt.h"
#include "console.h"

#define BUFFSIZE 512

void panic(char* message) {
    console_color(0xC7);

    kprintf("Kernel Panic - %s", message);
 
    while(true) {
        cli();
        hlt();
    }
}

void panicf(char* fmt, ...) {
    char buff[BUFFSIZE];
    va_list va;
    va_start(va, fmt);

    if(vsprintf(buff, fmt, va) > BUFFSIZE) {
        panic("panicf overflow");
    }

    va_end(va);

    panic(buff);
}
