#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#define MAX_CMD_LEN 256
#define MAX_NUM_ARGS 100

#define CSI_DOUBLE "\x1b["

static void send_backspace_esc() {
    printf(CSI_DOUBLE "D");
    printf(" ");
    printf(CSI_DOUBLE "D");
}

void print_welcome() {
    printf(CSI_DOUBLE "2J");
    printf(CSI_DOUBLE "1;1H");

    printf("Welcome to KSH!\n");
    printf("Copyright (c) 2018, Keeley Hoek.\n");
    printf("All rights reserved.\n\n");
}

void print_linestart() {
    printf("root@k-os:/$ ");
}

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
        execve(argv_tab[0], argv_tab, NULL);
        printf("%s: command not found\n", argv_tab[0]);
        _exit(1);
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
