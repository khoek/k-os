#include <stdlib.h>

#include <k/sys.h>

void exit(int status) {
  //FIXME do all the stuff we are meant to --- flush buffers, call registered
  //handlers, etc...

  _Exit(status);
}

void _Exit(int status) {
  SYSCALL(exit)(status);
}
