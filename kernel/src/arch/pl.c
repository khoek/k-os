#include "common/types.h"
#include "arch/pl.h"
#include "arch/gdt.h"
#include "arch/cpu.h"
#include "arch/proc.h"
#include "bug/debug.h"
#include "log/log.h"
#include "sched/task.h"

void enter_syscall() {
    check_irqs_disabled();

    barrier();
    irqenable();
}

void leave_syscall() {
    barrier();
    irqdisable();
}

void save_stack(void *sp) {
    check_irqs_disabled();

    BUG_ON(!current);
    BUG_ON(current->arch.live_state);

    current->arch.live_state = sp;
}

#define ALIGN_BITS 16

void switch_stack(thread_t *old, thread_t *next, void (*callback)(thread_t *old, thread_t *next)) {
    check_irqs_disabled();

    uint32_t new_esp = ((((uint32_t) next->arch.live_state) / ALIGN_BITS) - 1) * ALIGN_BITS;

    asm volatile("mov %[new_esp], %%esp\n"
                 "push %[next]\n"
                 "push %[old]\n"
                 "call *%[finish]"
        :
        : [new_esp] "r" (new_esp),
          [old] "r" (old),
          [next] "r" (next),
          [finish] "r" (callback)
        : "memory");

    BUG();
}

extern void do_context_switch(uint32_t cr3, cpu_state_t *live_state);

void context_switch(thread_t *t) {
    check_irqs_disabled();
    check_on_correct_stack();

    BUG_ON(!t);
    BUG_ON(!t->arch.live_state);

    tss_set_stack(t->kernel_stack_bottom);

    barrier();

    void *s = t->arch.live_state;
    t->arch.live_state = NULL;

    barrier();

    do_context_switch(t->arch.cr3, s);

    BUG();
}

#define kernel_stack_off(task, off) ((void *) (((uint32_t) (task)->kernel_stack_bottom) - (off)))

static inline void set_exec_state(exec_state_t *exec, void *ip, bool user) {
    exec->eip = (uint32_t) ip;

    if(user) {
        exec->eflags = EFLAGS_IF;
        exec->cs = SEL_USER_CODE | SPL_USER;
    } else {
        exec->eflags = 0;
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

static inline void set_live_state(thread_t *t, void *live_state) {
    BUG_ON(!live_state);
    t->arch.live_state = live_state;
}

//If reg==NULL we will just fill the registers with zeroes.
void pl_enter_userland(void *user_ip, void *user_stack, registers_t *reg) {
    thread_t *me = current;

    //We are about to build a register launchpad on the stack to jumpstart
    //execution at the user address.
    cpu_state_t user_state;

    //Build the register launchpad in user_state.
    set_reg_state(&user_state.reg, reg);
    set_exec_state(&user_state.exec, user_ip, true);
    set_stack_state(&user_state.stack, user_stack, true);

    //We can't be interrupted after clobbering live_state, else a task switch
    //interrupt will modify this value and we will end up who-knows-where?
    irqdisable();

    //Inform context_switch() of the register launchpad's location.
    set_live_state(me, &user_state);

    //Begin execution at user_ip (after some final chores).
    context_switch(me);
}

void pl_bootstrap_userland(void *user_ip, void *user_stack, uint32_t argc,
    void *argv, void *envp) {
    registers_t reg;
    memset(&reg, 0, sizeof(registers_t));
    reg.eax = argc;
    reg.ebx = (uint32_t) argv;
    reg.ecx = (uint32_t) envp;

    pl_enter_userland(user_ip, user_stack, &reg);
}

#define stack_alloc(stack, type) ({(stack) -= (sizeof(type)); ((type *) (stack));})

void pl_setup_thread(thread_t *t, void *ip, void *arg) {
    //It is absolutely imperative that "t" is not running, else the live_state
    //we install may be overwritten due to an interrupt.
    BUG_ON(t->state != THREAD_BUILDING);

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

void arch_thread_build(thread_t *t) {
    page_t *page = alloc_page(0);
    t->arch.dir = page_to_virt(page);
    t->arch.cr3 = (uint32_t) page_to_phys(page);
    build_page_dir(t->arch.dir);
}

typedef struct fork_data {
    cpu_state_t resume_state;
} fork_data_t;

void arch_ret_from_fork(void *arg) {
    check_irqs_disabled();

    fork_data_t forkd;
    memcpy(&forkd, arg, sizeof(fork_data_t));
    kfree(arg);

    forkd.resume_state.reg.edx = 0;
    forkd.resume_state.reg.eax = 0;

    pl_enter_userland((void *) forkd.resume_state.exec.eip, (void *) forkd.resume_state.stack.esp, &forkd.resume_state.reg);
}

void * arch_prepare_fork(cpu_state_t *state) {
    fork_data_t *forkd = kmalloc(sizeof(fork_data_t));
    memcpy(&forkd->resume_state, state, sizeof(cpu_state_t));
    return forkd;
}
