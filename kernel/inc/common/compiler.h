#ifndef KERNEL_COMMON_COMPILER_H
#define KERNEL_COMMON_COMPILER_H

#define ALIGN(a)    __attribute__((aligned((a))))
#define UNUSED(v) v __attribute__((unused))
#define PACKED      __attribute__((packed))
#define USED        __attribute__((used))

#define __forceinline __attribute__((always_inline))
#define __noreturn  __attribute__((noreturn))

#define STR(s)  #s
#define XSTR(s) STR(s)

#define STRLEN(s) ((sizeof(s) / sizeof(s[0])) - 1)

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &(((type *) 0)->member))
#endif

#define containerof(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type, member) );})

#define ACCESS_ONCE(x) (*((volatile typeof(x) *) &(x)))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif
