#define NULL ((void *) 0)

#include <k/sys.h>
#include <k/log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    while(1) {
        char c;
        read(0, &c, 1);
        write(0, &c, 1);
    }

    return 1;
}
