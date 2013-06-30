#ifndef KERNEL_ARCH_ATOMIC_OPS
#define KERNEL_ARCH_ATOMIC_OPS

#define __X86_CASE_B    1
#define __X86_CASE_W    2
#define __X86_CASE_L    4

#define xchg_op(op, ptr, arg)                                                                                   \
    ({                                                                                                          \
        __typeof__ (*(ptr)) __ret = (arg);                                                                      \
        switch (sizeof(*(ptr))) {                                                                               \
            case __X86_CASE_B:                                                                                  \
                asm volatile ("lock; x" #op "b %b0, %1\n" : "+q" (__ret), "+m" (*(ptr)) : : "memory", "cc");    \
                break;                                                                                          \
            case __X86_CASE_W:                                                                                  \
                asm volatile ("lock; x" #op "w %w0, %1\n" : "+r" (__ret), "+m" (*(ptr)) : : "memory", "cc");    \
                break;                                                                                          \
            case __X86_CASE_L:                                                                                  \
                asm volatile ("lock; x" #op "l %0, %1\n" : "+r" (__ret), "+m" (*(ptr)) : : "memory", "cc");     \
                break;                                                                                          \
        }                                                                                                       \
        __ret;                                                                                                  \
    })

#define add(ptr, inc)						                                                                    \
	({								                                                                            \
	        __typeof__ (*(ptr)) __ret = (inc);			                                                        \
		switch (sizeof(*(ptr))) {				                                                                \
		case __X86_CASE_B:					                                                                    \
			asm volatile ("lock; addb %b1, %0\n" : "+m" (*(ptr)) : "qi" (inc) : "memory", "cc");		        \
			break;						                                                                        \
		case __X86_CASE_W:					                                                                    \
			asm volatile ("lock; addw %w1, %0\n" : "+m" (*(ptr)) : "ri" (inc) : "memory", "cc");		        \
			break;						                                                                        \
		case __X86_CASE_L:					                                                                    \
			asm volatile ("lock; addl %1, %0\n" : "+m" (*(ptr)) : "ri" (inc) : "memory", "cc");		            \
			break;						                                                                        \
		}							                                                                            \
		__ret;                                                                                                  \
	})

#endif
