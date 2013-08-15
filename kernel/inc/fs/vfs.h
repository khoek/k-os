#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#define FILE_FLAG_STATIC (1 << 0)

typedef struct file file_t;
typedef struct file_ops file_ops_t;
typedef struct dentry dentry_t;

typedef struct disk disk_t;
typedef struct disk_ops disk_ops_t;
typedef struct disk_label disk_label_t;

#include "lib/int.h"
#include "common/list.h"

struct file {
    uint32_t flags;
    void *private;

    file_ops_t *ops;
};

struct file_ops {
    ssize_t (*seek)(file_t *file, size_t bytes);
    ssize_t (*read)(file_t *file, size_t bytes, void *buff);
    ssize_t (*write)(file_t *file, size_t bytes, void *buff);
};

struct dentry {
    char *filename;
    file_t *file;

    dentry_t *parent;
    list_head_t children;

    list_head_t list;
};

struct disk {
    disk_ops_t *ops;
    uint32_t block_size;

    disk_label_t *type;

    list_head_t list;
};

struct disk_ops {
    void (*seek)(disk_t *disk, size_t block);
    ssize_t (*read)(disk_t *disk, size_t start, size_t blocks, void *buff);
    ssize_t (*write)(disk_t *disk, size_t start, size_t blocks, void *buff);
};

struct disk_label {
    bool (*match)(disk_t *disk);
    bool (*probe)(disk_t *disk);

    list_head_t list;
};

void register_disk(disk_t *disk);
void register_disk_label(disk_label_t *disk_label);

#endif
