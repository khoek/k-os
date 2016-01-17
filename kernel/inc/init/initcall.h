#ifndef KERNEL_COMMON_INIT_H
#define KERNEL_COMMON_INIT_H

#define PHYSICAL_BASE 0x00100000
#define VIRTUAL_BASE  0xC0000000

#define ENTRY_AP_BASE 0x8000

#ifdef __LINKER__

#define INITCALL_SECTION(id) *(.init.call.##id)

#else

#define INITCALL int __init

#define __init     __attribute__ ((section(".init.text"), cold))
#define __initdata __attribute__ ((section(".init.data")))
#define __initconst __attribute__ ((constsection(".init.rodata")))

typedef int (*initcall_t)(void);

extern initcall_t initcall_start, initcall_end;

#define DEFINE_INITCALL(id, fn)                            \
	static initcall_t __initcall_##fn##id                  \
	__attribute__((section(".init.call." #id), used)) = fn

#define early_initcall(fn)    DEFINE_INITCALL(0, fn)
#define pure_initcall(fn)     DEFINE_INITCALL(1, fn)
#define core_initcall(fn)     DEFINE_INITCALL(2, fn)
#define postcore_initcall(fn) DEFINE_INITCALL(3, fn)
#define arch_initcall(fn)     DEFINE_INITCALL(4, fn)
#define postarch_initcall(fn) DEFINE_INITCALL(5, fn)
#define subsys_initcall(fn)   DEFINE_INITCALL(6, fn)
#define fs_initcall(fn)       DEFINE_INITCALL(7, fn)
#define postfs_initcall(fn)   DEFINE_INITCALL(8, fn)
#define device_initcall(fn)   DEFINE_INITCALL(9, fn)
#define module_initcall(fn)   DEFINE_INITCALL(10, fn)

#endif

#endif
