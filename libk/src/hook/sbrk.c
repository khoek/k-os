#include <unistd.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>
#include <k/math.h>

extern uint32_t __executable_start;
extern uint32_t _end;

void *_data_start = &__executable_start;
void *_data_end = &_end;

int brk(void *addr) {
    return ((int32_t) sbrk(((uint32_t) addr) - ((uint32_t) _data_end))) == -1 ? -1 : 0;
}

void * sbrk(ptrdiff_t incr) {
    if(incr < 0) {
        if((((uint32_t) _data_end) - ((uint32_t) _data_start)) > ((uint32_t) -incr)) {
            errno = EINVAL;
            return (void *) -1;
        } else {
            for(uint32_t i = 0; i < DIV_DOWN((uint32_t) _data_end, PAGE_SIZE) - DIV_DOWN(((uint32_t) _data_end) + incr, PAGE_SIZE); i++) {
                SYSCALL(free_page)();
            }
        }

        void *old_end = _data_end;
        _data_end = (void *) ((uint32_t) _data_end) - ((uint32_t) -incr);

        return old_end;
    } else if(incr > 0) {
        for(uint32_t i = 0; i < DIV_DOWN(((uint32_t) _data_end) + incr, PAGE_SIZE) - DIV_DOWN((uint32_t) _data_end, PAGE_SIZE); i++) {
            if(SYSCALL(alloc_page)(_data_end + PAGE_SIZE * (i + 1)) == -1) {
                int old_errno = errno;

                do {
                    i--;
                    if(SYSCALL(free_page)() == -1) break;
                } while(i > 0);

                errno = old_errno;

                //FIXME errno should have been set by SYSCALL(free_page)
                return (void *) -1;
            }
        }

        void *old_end = _data_end;
        _data_end = (void *) ((uint32_t) _data_end) + ((uint32_t) incr);

        return old_end;
    }

    return _data_end;
}
