#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

void wc(char c) {
    write(1, &c, 1);
}

char rc() {
    char c;
    read(0, &c, 1);
    return c;
}

#define CSI_DOUBLE_PARTA ((char) 0x1B)
#define CSI_DOUBLE_PARTB ((char) '[')

static void send_backspace_esc() {
    wc(CSI_DOUBLE_PARTA);
    wc(CSI_DOUBLE_PARTB);
    wc('D');

    wc(' ');

    wc(CSI_DOUBLE_PARTA);
    wc(CSI_DOUBLE_PARTB);
    wc('D');
}

void print_welcome() {
    wc(CSI_DOUBLE_PARTA);
    wc(CSI_DOUBLE_PARTB);
    wc('2');
    wc('J');

    wc(CSI_DOUBLE_PARTA);
    wc(CSI_DOUBLE_PARTB);
    wc('1');
    wc(';');
    wc('1');
    wc('H');

    printf("Welcome to KSH!\n");
    printf("Copyright (c) 2016, Keeley Hoek.\n");
    printf("All rights reserved.\n\n");
}

void print_linestart() {
    printf("root@k-os:/$ ");
    fflush(stdout);
}

char buff2[100];
char *buff[100];
void execute_command(char *raw) {
    char **argv_front = buff;

    strcpy(buff2, raw);
    raw = buff2;

    char *front = raw;
    while(*front && (*front == ' ')) front++;
    char *back = front;

    while(*front) {
        while(*front && (*front != ' ')) front++;

        *argv_front++ = back;

        //we hit the '\0' null character
        if(!*front) {
            break;
        }

        *front++ = '\0';
        while(*front && (*front == ' ')) front++;
        back = front;
    }
    *argv_front = NULL;

    if(!strcmp(buff[0], "exit")) {
        printf("exit\n");
        _exit(0);
    }

    pid_t cpid = fork();
    if(cpid) {
        waitpid(cpid, NULL, 0);
    } else {
        execve(buff[0], buff, NULL);
        printf("%s: command not found\n", buff[0]);
        _exit(1);
    }
}

int main(int argc, char **argv) {
    print_welcome();
    print_linestart();

    uint32_t off = 0;
    char buff[512];
    while(1) {
        char c = rc();

        switch(c) {
            case '\x7f': {
                if(off) {
                    off--;
                    send_backspace_esc();
                }

                break;
            }
            case '\n': {
                wc('\n');
                buff[off] = '\0';
                if(off) execute_command(buff);
                off = 0;
                print_linestart();

                break;
            }
            default: {
                wc(c);
                buff[off++] = c;

                break;
            }
        }
    }

    return 1;
}
