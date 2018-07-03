#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        printf("%s: missing operand\n", argv[0]);
        return 1;
    }

    sleep(atoi(argv[1]));

    return 0;
}
