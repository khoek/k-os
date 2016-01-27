#include "lib/int.h"
#include "arch/pl.h"
#include "arch/gdt.h"
#include "arch/cpu.h"
#include "bug/debug.h"
#include "sched/task.h"
#include "log/log.h"

static void overwrite_cpu_state(task_t *t) {
    memcpy(t->arch.live_state, &t->arch.cpu_state, sizeof(cpu_state_t));
}

void save_cpu_state(task_t *t, void *sp) {
    t->arch.live_state = sp;

    //Only save the iret/pusha stack data if a user->kernel switch occured.
    if(t->arch.live_state->exec.cs == (SEL_USER_CODE | SPL_USER)) {
        memcpy(&t->arch.cpu_state, t->arch.live_state, sizeof(cpu_state_t));
    }
}

static void load_cpu_state(task_t *t) {
    //Only restore the iret/pusha stack data if a user->kernel switch occured.
    if(t->arch.live_state->exec.cs == (SEL_USER_CODE | SPL_USER)) {
        overwrite_cpu_state(t);
    }
}

extern void do_context_switch(uint32_t cr3, cpu_state_t *live_state);

void context_switch(task_t *t) {
    tss_set_stack(t->kernel_stack_bottom);
    load_cpu_state(t);
    do_context_switch(t->arch.cr3, t->arch.live_state);
}

#define kernel_stack_off(task, off) ((void *) (((uint32_t) (task)->kernel_stack_bottom) - (off)))

static inline void pl_set_exec_state(task_t *task, void *ip, void *user_sp) {
    memset(&task->arch.cpu_state.reg, 0, sizeof(registers_t));

    exec_state_t *exec = &task->arch.cpu_state.exec;
    stack_state_t *stack = &task->arch.cpu_state.stack;

    exec->eflags = EFLAGS_IF;
    exec->eip = (uint32_t) ip;

    if(user_sp) {
        exec->cs = SEL_USER_CODE | SPL_USER;

        stack->ss = SEL_USER_DATA | SPL_USER;
        stack->esp = (uint32_t) user_sp;
    } else {
        exec->cs = SEL_KRNL_CODE | SPL_KRNL;

        //These should never be used
        stack->ss = SEL_KRNL_DATA | SPL_KRNL;
        stack->esp = 0;
    }
}

void pl_enter_userland(void *user_ip, void *user_stack) {
    //We are about to modify live_state, which is used to indicate the location
    //of the stack pointer for the popa then iret sequence. If an interrupt
    //occurs between this modification and the call to context_switch(), the
    //rug will be pulled from under us and live_state will be modified.
    cli();

    //Throw away the whole current stack and replace it with just the register
    //launchpad.
    current->arch.live_state = kernel_stack_off(current, sizeof(cpu_state_t));

    //Build the register launchpad in current->cpu_state.
    pl_set_exec_state(current, user_ip, user_stack);

    //Install the register launchapd in the stack.
    overwrite_cpu_state(current);

    //Begin execution at user_ip (after some final chores).
    context_switch(current);
}

void pl_setup_task(task_t *t, void *ip, void *arg) {
    //It is absolutely imperative that task is not running, else the live_state
    //we install may be overwritten due to an interrupt (not to mention damage
    //to arch.cpu_state).
    BUG_ON(t->state != TASK_BUILDING);

    *((void **) kernel_stack_off(t, sizeof(void *))) = arg;

    t->arch.live_state = kernel_stack_off(t, sizeof(cpu_launchpad_t) + sizeof(void *));

    pl_set_exec_state(t, ip, NULL);
    memcpy(t->arch.live_state, &t->arch.cpu_state, sizeof(cpu_launchpad_t));
}
