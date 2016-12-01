#ifndef KERNEL_ARCH_ATOMIC_OPS
#define KERNEL_ARCH_ATOMIC_OPS

#define __X86_CASE_B    1
#define __X86_CASE_W    2
#define __X86_CASE_L    4

#define xchg_op(op, ptr, arg) \
    ({                                                                        \
        __typeof__ (*(ptr)) __ret = (arg);                                    \
        switch (sizeof(*(ptr))) {                                             \
            case __X86_CASE_B:                                                \
                asm volatile ("lock; x" #op "b %b0, %1\n"                     \
                    : "+q" (__ret), "+m" (*(ptr))                             \
                    : : "memory", "cc");                                      \
                break;                                                        \
            case __X86_CASE_W:                                                \
                asm volatile ("lock; x" #op "w %w0, %1\n"                     \
                    : "+r" (__ret), "+m" (*(ptr))                             \
                    : : "memory", "cc");                                      \
                break;                                                        \
            case __X86_CASE_L:                                                \
                asm volatile ("lock; x" #op "l %0, %1\n"                      \
                    : "+r" (__ret), "+m" (*(ptr))                             \
                    : : "memory", "cc");                                      \
                break;                                                        \
        }                                                                     \
        __ret;                                                                \
    })

#define cmpxchg(ptr, old, new) __cmpxchg(ptr, old, new, sizeof(*ptr))

#define __cmpxchg(ptr, old, new, size)                                   \
    ({                                                                   \
         __typeof__(*(ptr)) __ret;                                       \
         __typeof__(*(ptr)) __old = (old);                               \
         __typeof__(*(ptr)) __new = (new);                               \
         switch (size) {                                                 \
         case __X86_CASE_B:                                              \
         {                                                               \
                 volatile uint8_t *__ptr = (volatile uint8_t *)(ptr);    \
                 asm volatile("lock; cmpxchgb %2,%1"                     \
                              : "=a" (__ret), "+m" (*__ptr)              \
                              : "q" (__new), "a" (__old)                  \
                              : "memory");                               \
                 break;                                                  \
         }                                                               \
         case __X86_CASE_W:                                              \
         {                                                               \
                 volatile uint16_t *__ptr = (volatile uint16_t *)(ptr);  \
                 asm volatile("lock; cmpxchgw %2,%1"                     \
                              : "=a" (__ret), "+m" (*__ptr)              \
                              : "r" (__new), "a" (__old)                  \
                              : "memory");                               \
                 break;                                                  \
         }                                                               \
         case __X86_CASE_L:                                              \
         {                                                               \
                 volatile uint32_t *__ptr = (volatile uint32_t *)(ptr);  \
                 asm volatile("lock; cmpxchgl %2,%1"                     \
                              : "=a" (__ret), "+m" (*__ptr)              \
                              : "r" (__new), "a" (__old)                  \
                              : "memory");                               \
                 break;                                                  \
         }                                                               \
         }                                                               \
        __ret;                                                           \
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
