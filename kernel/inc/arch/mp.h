#ifndef KERNEL_ARCH_MP_H
#define KERNEL_ARCH_MP_H

typedef struct processor processor_t;
extern processor_t *bsp;

#include "common/compiler.h"

void __noreturn mp_ap_init();
void __noreturn mp_run_cpu(processor_t *proc);

#endif
