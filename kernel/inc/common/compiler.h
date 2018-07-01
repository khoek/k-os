#ifndef KERNEL_COMMON_COMPILER_H
#define KERNEL_COMMON_COMPILER_H

#define ALIGN(a)    __attribute__((aligned((a))))
#define UNUSED(v) v __attribute__((unused))
#define PACKED      __attribute__((packed))
#define PURE        __attribute__((pure))

#define __used __attribute__((used))
#define __forceinline __attribute__((always_inline))
#define __noreturn  __attribute__((noreturn))

#define FALLTHROUGH __attribute__((fallthrough))

#define STR(s)  #s
#define XSTR(s) STR(s)

#define STRLEN(s) ((sizeof(s) / sizeof(s[0])) - 1)

#define EXPAND(x) x

#define _GET_NTH_ARG(_1, _2, _3, _4, _5, _6, _7, _9, _10, N, ...) N
#define NUM_ARGS(...) EXPAND(_GET_NTH_ARG(__VA_ARGS__,9,8,7,6,5,4,3,2,1,0))

#define REVERSE_0()
#define REVERSE_1(a) a
#define REVERSE_2(a,b) b,a
#define REVERSE_3(a,...) EXPAND(REVERSE_2(__VA_ARGS__)),a
#define REVERSE_4(a,...) EXPAND(REVERSE_3(__VA_ARGS__)),a
#define REVERSE_5(a,...) EXPAND(REVERSE_4(__VA_ARGS__)),a
#define REVERSE_6(a,...) EXPAND(REVERSE_5(__VA_ARGS__)),a
#define REVERSE_7(a,...) EXPAND(REVERSE_6(__VA_ARGS__)),a
#define REVERSE_8(a,...) EXPAND(REVERSE_7(__VA_ARGS__)),a
#define REVERSE_9(a,...) EXPAND(REVERSE_8(__VA_ARGS__)),a
#define REVERSE_10(a,...) EXPAND(REVERSE_9(__VA_ARGS__)),a
#define _REVERSE_GO(N,...) EXPAND(REVERSE_ ## N(__VA_ARGS__))
#define _REVERSE(N, ...) _REVERSE_GO(N, __VA_ARGS__)
#define REVERSE(...) _REVERSE(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &(((type *) 0)->member))
#endif

#define containerof(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type, member) );})

#define ACCESS_ONCE(x) (*((volatile typeof(x) *) &(x)))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define likely(x)   (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 0))

#endif
