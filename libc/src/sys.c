#include "k/sys.h"

void _msleep(uint32_t millis) {
  SYSCALL(msleep)(millis);
}
