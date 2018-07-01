#ifndef LIBC_K_SYSCALL_H
#define LIBC_K_SYSCALL_H

#define SYSCALL(name) __k_##name

#ifndef __ASSEMBLER__
#include "k/compiler.h"

#include "dirent.h"
#include "sys/types.h"
#include "sys/socket.h"

#define SYSCALL_SIG(name) int32_t SYSCALL(name)

#define DECLARE_SYSCALL(name, ...) SYSCALL_SIG(name)(__VA_ARGS__)

uint64_t perform_syscall_0(uint32_t id_num, ...);
uint64_t perform_syscall_1(uint32_t id_num, ...);
uint64_t perform_syscall_2(uint32_t id_num, ...);
uint64_t perform_syscall_3(uint32_t id_num, ...);
uint64_t perform_syscall_4(uint32_t id_num, ...);
uint64_t perform_syscall_5(uint32_t id_num, ...);

#include "k/shared/syscall_decls.h"
#endif

#endif
