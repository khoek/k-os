#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

#include <k/syscall.h>

int main(int argc, char **argv) {
    int fd = open(argv[1], 0);

    uint32_t count = 0;
    struct dirent ent;
    while(SYSCALL(readdir)(fd, &ent, 1) == 1) {
        count++;
        printf("%s ", ent.d_name);
    }
    if(count) {
      printf("\n");
    }

    return 0;
}
