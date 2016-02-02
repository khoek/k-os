#include "common/types.h"
#include "arch/pl.h"
#include "arch/gdt.h"
#include "arch/cpu.h"
#include "arch/proc.h"
#include "bug/debug.h"
#include "sched/task.h"

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

static inline void set_exec_state(exec_state_t *exec, void *ip, bool user) {
    exec->eflags = EFLAGS_IF;
    exec->eip = (uint32_t) ip;

    if(user) {
        exec->cs = SEL_USER_CODE | SPL_USER;
    } else {
        exec->cs = SEL_KRNL_CODE | SPL_KRNL;
    }
}

static inline void set_stack_state(stack_state_t *stack, void *sp, bool user) {
    if(user) {
        stack->ss = SEL_USER_DATA | SPL_USER;
        stack->esp = (uint32_t) sp;
    } else {
        //For kernel->kernel irets, ss and esp are not popped off.
        BUG();
    }
}

//If src==NULL we will just fill the registers with zeroes.
static inline void set_reg_state(registers_t *dst, registers_t *src) {
    if(src) {
        memcpy(dst, src, sizeof(registers_t));
    } else {
        memset(dst, 0, sizeof(registers_t));
    }
}

static inline void set_live_state(task_t *t, void *live_state) {
    t->arch.live_state = live_state;
}

//If reg==NULL we will just fill the registers with zeroes.
void pl_enter_userland(void *user_ip, void *user_stack, registers_t *reg) {
    task_t *me = current;

    //We are about to build a register launchpad on the stack to jumpstart
    //execution at the user address.
    cpu_state_t user_state;

    //Build the register launchpad in user_state.
    set_reg_state(&user_state.reg, reg);
    set_exec_state(&user_state.exec, user_ip, true);
    set_stack_state(&user_state.stack, user_stack, true);

    //We can't be interrupted after clobbering live_state, else a task switch
    //interrupt will modify this value and we will end up who-knows-where?
    cli();

    //Inform context_switch() of the register launchpad's location.
    set_live_state(me, &user_state);

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

    //Allocate space for the register launchpad on the new kernel stack.
    cpu_launchpad_t *state = stack_alloc(stack, cpu_launchpad_t);

    //Build the register launchpad.
    set_reg_state(&state->reg, NULL);
    set_exec_state(&state->exec, ip, false);

    //Inform context_switch() of the register launchpad's location.
    set_live_state(t, state);
}
