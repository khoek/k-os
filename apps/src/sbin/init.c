#define NULL ((void *) 0)

#include <unistd.h>

int main(int argc, char **argv) {
    char *shell_argv[] = {"/sbin/ksh", NULL};
    char *shell_envp[] = {NULL};
    execve("/sbin/ksh", shell_argv, shell_envp);

    return 1;
}
