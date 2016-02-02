#define NULL ((void *) 0)

#include <k/sys.h>
#include <k/log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void puts(char *str) {
    write(1, str, strlen(str));
}

void putc(char c) {
    write(1, &c, 1);
}

char getc() {
    char c;
    read(0, &c, 1);
    return c;
}

#define CSI_DOUBLE_PARTA ((char) 0x1B)
#define CSI_DOUBLE_PARTB ((char) '[')

static void send_backspace_esc() {
    putc(CSI_DOUBLE_PARTA);
    putc(CSI_DOUBLE_PARTB);
    putc('D');

    putc(' ');

    putc(CSI_DOUBLE_PARTA);
    putc(CSI_DOUBLE_PARTB);
    putc('D');
}

void print_welcome() {
    putc(CSI_DOUBLE_PARTA);
    putc(CSI_DOUBLE_PARTB);
    putc('2');
    putc('J');

    putc(CSI_DOUBLE_PARTA);
    putc(CSI_DOUBLE_PARTB);
    putc('1');
    putc(';');
    putc('1');
    putc('H');

    puts("Welcome to KSH!\n");
    puts("Copyright (c) 2016, Keeley Hoek.\n");
    puts("All rights reserved.\n\n");
}

void print_linestart() {
    puts("\nroot@k-os:/$ ");
}

void execute_command(char *cmd) {
    pid_t cpid = fork();

    if(!cpid) {
        execve(cmd, NULL, NULL);
        while(1);
    }
}

int main(int argc, char **argv) {
    print_welcome();
    print_linestart();

    uint32_t off = 0;
    char buff[512];
    while(1) {
        char c = getc();

        switch(c) {
            case '\x7f': {
                if(off) {
                    off--;
                    send_backspace_esc();
                }

                break;
            }
            case '\n': {
                buff[off] = '\0';
                if(off) execute_command(buff);
                off = 0;
                print_linestart();

                break;
            }
            default: {
                putc(c);
                buff[off++] = c;

                break;
            }
        }
    }

    return 1;
}
