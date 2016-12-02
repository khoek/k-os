#define NULL ((void *) 0)

#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

int main(int argc, char **argv) {
    char *shell_argv[] = {"/sbin/ksh", NULL};
    char *shell_envp[] = {NULL};

    while(1) {
        pid_t shell_id = fork();
        if(shell_id) {
            pid_t child;
            while((child = wait(NULL)) != shell_id);
        } else {
            execve("/sbin/ksh", shell_argv, shell_envp);
        }
    }

    return 1;
}
