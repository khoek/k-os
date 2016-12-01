#define NULL ((void *) 0)

#include <unistd.h>
#include <wait.h>
#include <string.h>

int main(int argc, char **argv) {
    char *shell_argv[] = {"/sbin/ksh", NULL};
    char *shell_envp[] = {NULL};

    pid_t cpid = fork();
    if(cpid) {
        while(wait(NULL));
    } else {
        execve("/sbin/ksh", shell_argv, shell_envp);
    }

    return 1;
}
