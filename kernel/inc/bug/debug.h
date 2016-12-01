#ifndef KERNEL_BUG_DEBUG_H
#define KERNEL_BUG_DEBUG_H

#define MAX_STACK_FRAMES 25

#define UNIMPLEMENTED() do { panicf("Unimplemented! @ %s:%u", __FILE__, __LINE__); } while(0)

#define NOP() do {} while(0)

#define BUG() do { panicf("Bugcheck failed in %s:%u", __FILE__, __LINE__); } while(0)

#ifdef CONFIG_DEBUG_BUGCHECKS
    #define BUG_ON(c) do { if((c)) BUG(); } while(0)

    #define breakpoint() do { breakpoint_triggered(); } while(0)

    #define check_on_correct_stack() do {                                   \
        thread_t *t = current;                                              \
        if(t) {                                                             \
            void *esp = get_sp();                                           \
            if(esp < t->kernel_stack_top || esp > t->kernel_stack_bottom) { \
                panicf("running on bad stack! esp=%X", esp);                \
            }                                                               \
        } else if(tasking_up) {                                             \
            panic("null task pointer!");                                    \
        }                                                                   \
    } while(0)

    #define check_irqs_disabled() do { if(are_interrupts_enabled()) panicf("IRQs enabled illegally!"); } while(0)
#else
    #define BUG_ON(c) do { NOP(); (void)(c);} while(0)

    #define breakpoint() NOP()
    #define check_on_correct_stack() NOP()
    #define check_irqs_disabled() NOP()
#endif

#include "bug/panic.h"
#include "fs/elf.h"
#include "driver/console/console.h"

void breakpoint_triggered();

void dump_stack_trace(console_t *t);

void debug_map_virtual();
const elf_symbol_t * debug_lookup_symbol(uint32_t address);
const char * debug_symbol_name(const elf_symbol_t *symbol);

void debug_init();

#endif
