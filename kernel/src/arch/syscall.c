#include "sched/syscall.h"
#include "arch/gdt.h"
#include "init/initcall.h"

static void syscall_handler(interrupt_t *interrupt, void *data) {
    enter_syscall();

    cpu_state_t *state = &interrupt->cpu;

    if(state->reg.eax >= MAX_SYSCALL || !syscalls[state->reg.eax]) {
        panicf("Unregistered Syscall #%u", state->reg.eax);
    } else {
        uint64_t ret = syscalls[state->reg.eax]
          (state, state->reg.ecx, state->reg.edx, state->reg.ebx,
            state->reg.esi, state->reg.edi);
        /*MISSING: state->reg.ebp, state->reg.esp, state->reg.eax*/

        state->reg.edx = ret >> 32;
        state->reg.eax = ret;
    }

    leave_syscall();

    //This might not return (as in S_DIE).
    deliver_signals();
}

static INITCALL syscall_init() {
    register_isr(SYSCALL_INT, CPL_USER, syscall_handler, NULL);

    return 0;
}

core_initcall(syscall_init);
