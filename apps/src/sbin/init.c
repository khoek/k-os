#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define SHELL_PATH "/bin/bash"
#define TERMINAL_NAME "kosterm"

#define CSI_DOUBLE "\x1b["

void clear_screen() {
    printf(CSI_DOUBLE "2J");
    printf(CSI_DOUBLE "1;1H");
    fflush(stdout);
}

int main(int argc, char **argv) {
    clear_screen();

    char *shell_argv[] = {SHELL_PATH, NULL};
    char *shell_envp[] = {"TERM=" TERMINAL_NAME, NULL};

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
            execve(SHELL_PATH, shell_argv, shell_envp);
            perror("init");
            //FIXME send kill signal to parent process to trigger a panic
            while(1);
        }
    }

    return 1;
}
