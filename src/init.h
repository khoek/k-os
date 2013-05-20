#ifndef KERNEL_INIT_H
#define KERNEL_INIT_H

#define INITCALL_PURE 0
#define INITCALL_PURE 0
#define INITCALL_CORE 1

#ifdef __LINKER__

#define SECTION_INITCALL_LEVEL(id) *(.initcall.##id##)

#else

#define __init     __section(.init.text) __cold notrace
#define __initdata __section(.init.data)

typedef int (*initcall_t)(void);

#define INITCALL(id, fn) \
	static initcall_t __initcall_##fn##id __used \
	__attribute__((__section__(".initcall." #id))) = fn

#endif

#endif
