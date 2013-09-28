#include "lib/int.h"
#include "common/asm.h"
#include "input/keyboard.h"
#include "video/log.h"
#include "misc/sysrq.h"

char fake_idt = 0;

void sysrq_handle(uint16_t code) {
    switch(code) {
        case B_KEY: {
            lidt(&fake_idt);
            asm("int $0x80");
            break;
        }
        case C_KEY: {
            *((char *) 0) = 0;
            break;
        }
    }
}
