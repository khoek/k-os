#include "lib/int.h"
#include "common/asm.h"
#include "common/compiler.h"
#include "video/log.h"
#include "misc/stats.h"
#include "misc/sysrq.h"
#include "driver/console/console.h"

static char fake_idt;

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
        case S_KEY: {
            kprintf("stats:");
            kprintf("%u tasks", task_count);
            kprintf("%u file descriptors in use", gfds_in_use);
            kprintf("%u/%u pages allocated/avaliable", pages_in_use, pages_avaliable);
            break;
        }
    }
}
