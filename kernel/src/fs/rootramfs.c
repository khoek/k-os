#include "common/types.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "fs/vfs.h"
#include "log/log.h"

#define MAGIC "KRFS"

#define ENTRY_TYPE_DIR      1
#define ENTRY_TYPE_FRECORD  2

typedef struct frecord {
    uint32_t len;
    uint8_t data[];
} PACKED frecord_t;

typedef struct entry {
    uint8_t type;
    uint32_t name_len;
    char name[];
} PACKED entry_t;

typedef struct rootramfs_header {
    char magic[4];
} PACKED rootramfs_header_t;

static uint32_t entry_reconstruct(entry_t *e, const path_t *root) {
    switch(e->type) {
        case ENTRY_TYPE_DIR: {
            char *pathname = kmalloc(e->name_len + 2);
            memcpy(pathname, e->name, e->name_len);
            pathname[e->name_len] = '/';
            pathname[e->name_len + 1] = '\0';

            kprintf("rootramfs - loading \"/%s\"", pathname);

            char *part = pathname;
            while(*part) {
                part = strchr(part, '/');

                if(!part) break;

                part[0] = '\0';

                path_t path;
                int32_t ret = vfs_create(root, pathname, S_IFDIR | 0755, &path);
                if(ret < 0 && ret != -EEXIST) {
                    panicf("rootramfs - vfs_create() failed: %d", ret);
                }

                part[0] = '/';
                part++;
            }

            return sizeof(entry_t) + e->name_len;
        }
        case ENTRY_TYPE_FRECORD: {
            char *pathname = kmalloc(e->name_len + 1);
            memcpy(pathname, e->name, e->name_len);
            pathname[e->name_len] = '\0';

            kprintf("rootramfs - loading \"/%s\"", pathname);

            path_t path;
            int32_t ret = vfs_create(root, pathname, S_IFREG | 0755, &path);
            if(ret < 0) {
                panicf("rootramfs - vfs_create() failed: %d", ret);
            }

            frecord_t *fr = ((void *) e) + sizeof(entry_t) + e->name_len;

            file_t *f = vfs_open_file(&path);
            vfs_write(f, fr->data, fr->len);

            return sizeof(entry_t) + e->name_len + sizeof(frecord_t) + fr->len;
        }
        default: {
            BUG();

            break;
        }
    }
}

void rootramfs_load(void *start, uint32_t len) {
    rootramfs_header_t *hdr = start;
    if(memcmp(&hdr->magic, MAGIC, 4)) {
        panic("rootramfs - invalid magic hdr");
    }

    uint32_t off = sizeof(rootramfs_header_t);
    while(off < len) {
        off += entry_reconstruct(start + off, &MNT_ROOT(root_mount));
    }
}
