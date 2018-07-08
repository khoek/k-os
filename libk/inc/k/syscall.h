#ifndef LIBK_K_SYSCALL_H
#define LIBK_K_SYSCALL_H

#define SYSCALL_NAME(name) __k_##name

#ifdef __ASSEMBLER__
  #define SYSCALL(name) SYSCALL_NAME(name)
#else
  #include "k/compiler.h"
  #include "k/types.h"

  #include <stdbool.h>
  #include "dirent.h"
  #include "sys/socket.h"

  #define SYSCALL_SIG(name) int32_t SYSCALL_NAME(name)

  #define DECLARE_SYSCALL(name, ...) SYSCALL_SIG(name)(__VA_ARGS__)

  uint64_t perform_syscall_0(uint32_t id_num, ...);
  uint64_t perform_syscall_1(uint32_t id_num, ...);
  uint64_t perform_syscall_2(uint32_t id_num, ...);
  uint64_t perform_syscall_3(uint32_t id_num, ...);
  uint64_t perform_syscall_4(uint32_t id_num, ...);
  uint64_t perform_syscall_5(uint32_t id_num, ...);

  #include <errno.h>

  #define MAKE_SYSCALL(name, ...) ({                \
      int __ret = SYSCALL_NAME(name)(__VA_ARGS__);  \
      if(__ret < 0) {                               \
        errno = -__ret;                             \
        __ret = -1;                                 \
      }                                             \
      __ret;                                        \
    })

  #include "k/shared/syscall_decls.h"
#endif

#endif
