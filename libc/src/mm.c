#include <k/sys.h>
#include <k/math.h>

#include <errno.h>
#include <unistd.h>

void *_data_start;
void *_data_end;

int brk(void *addr) {
    return ((int32_t) sbrk(((uint32_t) addr) - ((uint32_t) _data_end))) == -1 ? -1 : 0;
}

void * sbrk(intptr_t incr) {
    if(incr < 0) {
        if((((uint32_t) _data_end) - ((uint32_t) _data_start)) > ((uint32_t) -incr)) {
            //FIXME errno = EINVAL
            return (void *) -1;
        } else {
            for(uint32_t i = 0; i < DIV_UP((uint32_t) _data_end, PAGE_SIZE) - DIV_UP(((uint32_t) _data_end) + incr, PAGE_SIZE); i++) {
                _free_page();
            }
        }

        void *old_end = _data_end;
        _data_end = (void *) ((uint32_t) _data_end) - ((uint32_t) -incr);

        return old_end;
    } else if(incr > 0) {
        for(uint32_t i = 0; i < DIV_UP(((uint32_t) _data_end) + incr, PAGE_SIZE) - DIV_UP((uint32_t) _data_end, PAGE_SIZE); i++) {
            if(_alloc_page() == -1) {
                errno_t old_errno = errno;

                do {
                    i--;
                    if(_free_page() == -1) break;
                } while(i > 0);

                errno = old_errno;

                //errno should have been set by _free_page
                return (void *) -1;
            }
        }

        void *old_end = _data_end;
        _data_end = (void *) ((uint32_t) _data_end) + ((uint32_t) incr);

        return old_end;
    }

    return _data_end;
}
