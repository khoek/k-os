#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_CMD_LEN 256
#define MAX_NUM_ARGS 100

#define CSI_DOUBLE "\x1b["

#define PATH_SEPARATOR ':'
#define DIRECTORY_SEPARATOR '/'

static void send_backspace_esc() {
    printf(CSI_DOUBLE "D");
    printf(" ");
    printf(CSI_DOUBLE "D");
}

void print_welcome() {
    printf("Welcome to KSH!\n");
    printf("Copyright (c) 2018, Keeley Hoek.\n");
    printf("All rights reserved.\n\n");
}

void print_linestart() {
    printf("root@k-os:/$ ");
}

char * try_execve(char *path_part, char **argv_tab, uint32_t cmd_len) {
    char *envp_tab[] = { NULL };
    char path_buff[MAX_CMD_LEN + 1];

    uint32_t count = 0;
    while(*path_part && *path_part != PATH_SEPARATOR && count < MAX_CMD_LEN) {
        path_buff[count++] = *path_part++;
    }

    if(count + 1 + cmd_len >= MAX_CMD_LEN) {
        path_buff[count] = 0;
        printf("path lookup too long: %s/%s\n", path_buff, argv_tab[0]);
        exit(1);
    }

    // If path_part was nontrivial and doesn't end in a DIRECTORY_SEPARATOR, add
    // one.
    if(count && path_buff[count - 1] != DIRECTORY_SEPARATOR) {
        path_buff[count++] = DIRECTORY_SEPARATOR;
    }

    memcpy(path_buff + count, argv_tab[0], cmd_len + 1);

    int fd = open(path_buff, 0);
    if(fd != -1) {
        fexecve(fd, argv_tab, envp_tab);
    }

    return *path_part ? path_part + 1 : NULL;
}

char *PATH = "/usr/sbin:/usr/bin:/sbin:/bin";

void execute_command(char *raw) {
    char cmd_buff[MAX_CMD_LEN + 1];
    char *argv_tab[MAX_NUM_ARGS + 1]; // + 1 for null at end of table

    if(strlen(raw) > MAX_CMD_LEN) {
        printf("command too long!\n");
        return;
    }

    strcpy(cmd_buff, raw);
    raw = cmd_buff;

    uint32_t num_args = 0;
    char *front = raw;
    while(*front && (*front == ' ')) front++;
    char *back = front;

    while(*front) {
        if(num_args >= MAX_NUM_ARGS) {
            printf("too many args in command!\n");
            return;
        }

        while(*front && (*front != ' ')) front++;

        argv_tab[num_args++] = back;

        //we hit the '\0' null character
        if(!*front) {
            break;
        }

        *front++ = '\0';
        while(*front && (*front == ' ')) front++;
        back = front;
    }
    argv_tab[num_args] = NULL;

    if(!strcmp(argv_tab[0], "exit")) {
        printf("exit\n");
        _exit(0);
    }

    pid_t cpid = fork();
    if(cpid) {
        waitpid(cpid, NULL, 0);
    } else {
        uint32_t cmd_len = strlen(argv_tab[0]);

        try_execve("", argv_tab, cmd_len);

        bool cmd_contains_sep = strchr(argv_tab[0], DIRECTORY_SEPARATOR) != NULL;
        if(!cmd_contains_sep) {
            char *next = PATH;
            while((next = try_execve(next, argv_tab, cmd_len)));
        }

        printf("%s: command not found\n", argv_tab[0]);
        exit(1);
    }
}

int main(int argc, char **argv) {
    print_welcome();
    print_linestart();

    uint32_t off = 0;
    char buff[MAX_CMD_LEN + 1];
    while(1) {
        fflush(stdout);

        int c = getchar();
        switch(c) {
            case '\x7f': {
                if(off) {
                    off--;
                    send_backspace_esc();
                }

                break;
            }
            case '\n': {
                putchar('\n');
                buff[off] = '\0';
                if(off) execute_command(buff);
                off = 0;
                print_linestart();

                break;
            }
            case -1: {
                printf("EOF\n");
                return 0;
            }
            default: {
                if(off < MAX_CMD_LEN) {
                    buff[off++] = c;
                    putchar(c);
                }

                break;
            }
        }
    }

    return 1;
}
