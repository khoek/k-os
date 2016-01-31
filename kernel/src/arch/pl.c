#include "common/types.h"
#include "arch/pl.h"
#include "arch/gdt.h"
#include "arch/cpu.h"
#include "bug/debug.h"
#include "sched/task.h"
#include "log/log.h"

void enter_syscall() {
    BUG_ON(get_eflags() & EFLAGS_IF);

    barrier();
    sti();
}

void leave_syscall() {
    barrier();
    cli();
}

void save_stack(void *sp) {
    BUG_ON(!current);
    current->arch.live_state = sp;
}

extern void do_context_switch(uint32_t cr3, cpu_state_t *live_state);

void context_switch(task_t *t) {
    tss_set_stack(t->kernel_stack_bottom);
    do_context_switch(t->arch.cr3, t->arch.live_state);
}

#define kernel_stack_off(task, off) ((void *) (((uint32_t) (task)->kernel_stack_bottom) - (off)))

static inline void pl_set_state(cpu_state_t *state, void *ip, void *user_sp) {
    memset(&state->reg, 0, sizeof(registers_t));

    state->exec.eflags = EFLAGS_IF;
    state->exec.eip = (uint32_t) ip;

    if(user_sp) {
        state->exec.cs = SEL_USER_CODE | SPL_USER;

        state->stack.ss = SEL_USER_DATA | SPL_USER;
        state->stack.esp = (uint32_t) user_sp;
    } else {
        state->exec.cs = SEL_KRNL_CODE | SPL_KRNL;

        //It is critical that we don't write to state->stack.ss/esp here. They
        //may not correspond to a valid memory location, as if user_sp == NULL
        //"state" points to a cpu_launchpad_t.
    }
}

void pl_enter_userland(void *user_ip, void *user_stack) {
    task_t *me = current;

    //We are about to build a register launchpad on the stack to jumpstart
    //execution at the user address.
    cpu_state_t user_state;

    //Build the register launchpad in user_state.
    pl_set_state(&user_state, user_ip, user_stack);

    //We can't be interrupted after clobbering live_state, else a task switch
    //interrupt will modify this value and we will end up who-knows-where?
    cli();

    //Inform context_switch() of the register launchpad.
    me->arch.live_state = &user_state;

    //Begin execution at user_ip (after some final chores).
    context_switch(me);
}

#define stack_alloc(stack, type) ({(stack) -= (sizeof(type)); ((type *) (stack));})

void pl_setup_task(task_t *t, void *ip, void *arg) {
    //It is absolutely imperative that "t" is not running, else the live_state
    //we install may be overwritten due to an interrupt (not to mention damage
    //to arch.cpu_state).
    BUG_ON(t->state != TASK_BUILDING);

    void *stack = t->kernel_stack_bottom;

    //Ensure 16 byte alignment of stack frame. I don't know why the argument is
    //8 byte aligned.
    stack_alloc(stack, void *);
    stack_alloc(stack, void *);
    *stack_alloc(stack, void *) = arg;
    stack_alloc(stack, void *);

    cpu_state_t *state = (void *) stack_alloc(stack, cpu_launchpad_t);

    pl_set_state(state, ip, NULL);

    t->arch.live_state = state;
}
