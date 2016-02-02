#define NULL ((void *) 0)

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void puts(char *str) {
    write(1, str, strlen(str));
}

int main(int argc, char **argv) {
    puts("Hello, world!\n");

    return 0;
}
