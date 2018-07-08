#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define CSI_DOUBLE "\x1b["

void clear_screen() {
    printf(CSI_DOUBLE "2J");
    printf(CSI_DOUBLE "1;1H");
    fflush(stdout);
}

int main(int argc, char **argv) {
    clear_screen();

    char *shell_argv[] = {"/sbin/ksh", NULL};
    char *shell_envp[] = {NULL};

    bool first_shell = true;
    while(1) {
        pid_t shell_id = fork();
        if(shell_id) {
            pid_t child;
            while((child = wait(NULL)) != shell_id);
            first_shell = false;
        } else {
            if(!first_shell) {
                printf("\nShell died, respawning!\n");
            }

            execve("/sbin/bash", shell_argv, shell_envp);
        }
    }

    return 1;
}
