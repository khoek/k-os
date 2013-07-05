#ifndef KERNEL_COMPILER_H
#define KERNEL_COMPILER_H

#define ACCESS_ONCE(x) (*((volatile typeof(x) *) &(x)))

#endif
