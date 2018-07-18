#include "arch/gdt.h"
#include "sched/syscall.h"
#include "sched/sched.h"
#include "init/initcall.h"

static void syscall_handler(interrupt_t *interrupt, void *data) {
    enter_syscall();

    cpu_state_t *state = &interrupt->cpu;
    uint32_t num = state->reg.eax;

    if(num >= MAX_SYSCALL || !syscalls[num]) {
        panicf("Unregistered Syscall #%u", num);
    } else {
        uint64_t ret = syscalls[num](state, state->reg.ecx, state->reg.edx,
            state->reg.ebx, state->reg.esi, state->reg.edi);
        /*MISSING: state->reg.ebp, state->reg.esp, num*/

        //sys__sigreturn() restores state, and doesn't return a value!
        if(num != NSYS__SIGRETURN) {
            state->reg.edx = ret >> 32;
            state->reg.eax = ret;
        }
    }

    leave_syscall();
}

static INITCALL syscall_init() {
    register_isr(SYSCALL_INT, CPL_USER, syscall_handler, NULL);

    return 0;
}

core_initcall(syscall_init);
