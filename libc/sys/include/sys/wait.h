#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define WNOHANG 1
#define WUNTRACED 2

/* A status looks like:
      <1 byte info> <1 byte code>

      <code> == 0, child has exited, info is the exit value
      <code> & 0x7f == 1, child has exited, info is the signal number.
      <code> & 0x7f == 7f, child has stopped, info is the signal number.
      <code> & 0x80, there was a core dump.
*/

#define WIFEXITED(w)   (((w) & 0xff) == 0)
#define WIFSIGNALED(w) (((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)  (((w) & 0xff) == 0x7f)
#define WCOREDUMP(w)   ((w) & 0x80)
#define WEXITSTATUS(w) (((w) >> 8) & 0xff)
#define __WGETSIG(w)   (WEXITSTATUS(w) & 0x7f)
#define WTERMSIG       __WGETSIG
#define WSTOPSIG       __WGETSIG

pid_t wait (int *);
pid_t waitpid (pid_t, int *, int);

#ifdef _COMPILING_NEWLIB
pid_t _wait (int *);
#endif

/* Provide prototypes for most of the _<systemcall> names that are
   provided in newlib for some compilers.  */
pid_t _wait (int *);

#ifdef __cplusplus
};
#endif

#endif
