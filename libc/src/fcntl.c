#include "fcntl.h"

#include "k/sys.h"

int open(const char *path, int oflag, ...) {
  return SYSCALL(open)(path, oflag);
}
