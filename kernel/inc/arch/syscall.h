#ifndef KERNEL_ARCH_SYSCALL_H
#define KERNEL_ARCH_SYSCALL_H

#define MAX_SYSCALL 256

#define SYSCALL_INT 0x80

#include "common/types.h"
#include "arch/cpu.h"
#include "net/socket.h"

/*Missing: uint32_t ebp, uint32_t esp, uint32_t eax*/
typedef uint64_t (*syscall_t)(void* state, uint32_t ecx, uint32_t edx, uint32_t ebx, uint32_t esi, uint32_t edi);

#define SYSCALL(name) sys_##name
#define SYSCALL_NUM(name) __sys_##name##_id
#define SYSCALL_SIG(name) uint64_t SYSCALL(name)

#define DECLARE_SYSCALL(name, ...) SYSCALL_SIG(name)(cpu_state_t *state __VA_OPT__(,) __VA_ARGS__)
#define DEFINE_SYSCALL(name, ...) DECLARE_SYSCALL(name, __VA_ARGS__)

#define SYSCALL_ENTRY(num, name) [num] = (syscall_t) (void *) SYSCALL(name)

#endif
