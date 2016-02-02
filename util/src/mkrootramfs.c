#define _XOPEN_SOURCE 500

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define BUFF_SIZE (1ULL << 20)

#define NUM_FDS 256

#define MAGIC "KRFS"

#define ENTRY_TYPE_DIR      1
#define ENTRY_TYPE_FRECORD  2

#define PACKED __attribute__((packed))

typedef struct frecord {
    uint32_t len;
} PACKED frecord_t;

typedef struct entry {
    uint8_t type;
    uint32_t name_len;
} PACKED entry_t;

typedef struct rootramfs_header {
    char magic[4];
} PACKED rootramfs_header_t;

static void print_usage() {
    fprintf(stderr, "usage: mkrootramfs [options] in-dir\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "    -o outfile\t\tSpecifies the path of the output file.\n \t\t\tDefaults to \"rootramfs\".\n");
}

static bool is_dir(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

#define PATH_SEPARATOR '/'

static void *buff;
static uint32_t off;

static void header_alloc() {
    rootramfs_header_t *h = buff + off;
    memcpy(h->magic, MAGIC, strlen(MAGIC));
    off += sizeof(rootramfs_header_t);
}

static void entry_alloc(const char *filepath, uint8_t type) {
    entry_t *e = buff + off;
    off += sizeof(entry_t);

    e->type = type;
    e->name_len = strlen(filepath);
    memcpy(buff + off, filepath, e->name_len);
    off += e->name_len;
}

static void frecord_alloc(const char *real_filepath) {
    frecord_t *f = buff + off;
    off += sizeof(frecord_t);

    FILE *inf = fopen(real_filepath, "rb");
    struct stat buf;
    fstat(fileno(inf), &buf);
    int32_t b = fread(buff + off, sizeof(uint8_t), buf.st_size, inf);
    if(b != buf.st_size) {
        printf("could not read file \"%s\"! skipping...\n", real_filepath);
        off -= sizeof(frecord_t);
    }
    fclose(inf);
    off += buf.st_size;

    f->len = buf.st_size;
}

static int build_entry(const char *orig_filepath, const struct stat *info, const int typeflag, struct FTW *pathinfo) {
    const char *filepath = orig_filepath;
    while(*filepath) {
        if(*filepath == PATH_SEPARATOR) {
            filepath++;
            break;
        }

        filepath++;
    }

    if(!*filepath) return 0;

    if(typeflag == FTW_F) {
        entry_alloc(filepath, ENTRY_TYPE_FRECORD);
        frecord_alloc(orig_filepath);
    } else if(typeflag == FTW_D || typeflag == FTW_DP) {
        entry_alloc(filepath, ENTRY_TYPE_DIR);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char *outpath = "rootramfs", *indir;

    char c;
    while((c = getopt (argc, argv, "o:")) != -1) switch (c) {
        case 'o':
            outpath = optarg;
            break;
        case 'h':
        case '?':
        default:
            print_usage();
            return 1;
    }

    if(optind == argc) {
        fprintf(stderr, "error: you must supply an input directory\n");
        print_usage();
        return 1;
    }

    indir = argv[optind];

    if(!is_dir(indir)) {
        fprintf(stderr, "error: \"%s\" is not a directory\n", indir);
        print_usage();
        return 1;
    }

    buff = malloc(BUFF_SIZE);
    header_alloc();

    nftw(indir, build_entry, NUM_FDS, FTW_PHYS);

    FILE *outf = fopen(outpath, "wb");
    fwrite(buff, sizeof(uint8_t), off, outf);
    fclose(outf);

    return 0;
}
