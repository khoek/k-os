#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

#ifndef KERNEL_OPTIMIZE

#include "panic.h"
#define DEPENDS(m, c) if(!(c)) panic("Dependency assertion on \"%s\" failed!", m)
#define ASSERT(c)     if(!(c)) panic("Assertion failed!")
#define BUG_ON(c)     if(!(c)) panic("Bug!")

#else

#define NOP do{}while(0)

#define DEPENDS(m, c) NOP
#define ASSERT(c)     NOP
#define BUG_ON(c)     NOP

#endif

#endif
