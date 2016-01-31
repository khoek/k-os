#define NULL ((void *) 0)

#include <k/sys.h>
#include <k/log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void dowrite(int fd, char *str) {
    write(fd, str, strlen(str));
}

int main() {
    for(int i = 0; i < 25; i++) {
        dowrite(0, "\n");
    }

    while(1) {
        char c;
        read(0, &c, 1);
        write(0, &c, 1);
    }

    return 1;
}
