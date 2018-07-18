#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <k/sys.h>
#include <k/math.h>

extern uint32_t __executable_start;
extern uint32_t _end;

#define IMAGE_START ((uint32_t) &__executable_start)
#define IMAGE_END ((uint32_t) &_end)

#define ERR_PTR ((void *) -1)

static uint32_t brk_loc = IMAGE_END;

int brk(void *addr) {
    if(((uint32_t) brk_loc) > INT32_MAX || ((uint32_t) addr) > INT32_MAX) {
        return -ENOMEM;
    }

    int32_t new_brk = ((int32_t) addr);
    return sbrk(new_brk - brk_loc) == ERR_PTR ? -1 : 0;
}

void * sbrk(ptrdiff_t incr) {
    uint32_t data_end = brk_loc;
    uint32_t new_data_end = data_end + incr;

    uint32_t end_page = DIV_DOWN(data_end - 1, PAGE_SIZE);
    uint32_t new_end_page = DIV_DOWN(new_data_end - 1, PAGE_SIZE);
    if(incr < 0) {
        if(new_data_end < IMAGE_END || new_data_end > data_end) {
            errno = EINVAL;
            return ERR_PTR;
        }

        for(uint32_t i = end_page + 1; i >= new_end_page; i--) {
            if(MAKE_SYSCALL(free_page, i) < 0) {
                // printf("free_page() failure!");
                exit(1);
            }
        }
    } else if(incr > 0) {
        if(new_data_end < data_end) {
            errno = ENOMEM;
            return ERR_PTR;
        }

        for(uint32_t i = end_page + 1; i <= new_end_page; i++) {
            if(MAKE_SYSCALL(alloc_page, i) < 0) {
                // printf("alloc_page() failure!");
                exit(1);
            }
        }
    }

    uint32_t old_brk_loc = brk_loc;
    brk_loc = new_data_end;
    return (void *) old_brk_loc;
}
