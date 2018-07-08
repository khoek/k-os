#include <signal.h>
#include <k/sys.h>

int kill(int pid, int sig) {
    return MAKE_SYSCALL(unimplemented, "kill", true);
}
