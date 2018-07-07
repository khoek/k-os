#include <stdio.h>

int main(int argc, char **argv) {
    if(argc == 0) {
        int c;
        while((c = getchar()) != -1) {
            putchar(c);
        }
    } else {
        for(int i = 1; i <= argc; i++) {
            printf("unimplemented!");
            return 1;
        }
    }

    return 0;
}
