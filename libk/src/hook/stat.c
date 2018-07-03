#include <sys/stat.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int stat (const char *file, struct stat *st) {
  return SYSCALL(stat)(file, st);
}
