#include <stdio.h>
#include <dirent.h>

int main(int argc, char **argv) {
    if(argc <= 1) {
        argv[1] = ".";
    }
    DIR *dir = opendir(argv[1]);
    if(dir == NULL) {
        printf("%s: cannot access '%s'\n", argv[0], argv[1]);
        return 1;
    }

    uint32_t count = 0;
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        count++;
        printf("%s ", ent->d_name);
    }
    if(count) {
      printf("\n");
    }

    return 0;
}
