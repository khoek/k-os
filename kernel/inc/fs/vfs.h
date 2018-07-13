#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#define DIRECTORY_SEPARATOR '/'
#define DIRECTORY_SEPARATOR_STR "/"

#define FSTYPE_FLAG_NODEV (1 << 0)

#define FS_FLAG_STATIC (1 << 0)
#define FS_FLAG_NODEV  (1 << 1)

#define INODE_FLAG_DIRECTORY  (1 << 0)
#define INODE_FLAG_MOUNTPOINT (1 << 1)

#define MNT_ROOT(mnt) ((path_t) {.mount = mnt, .dentry = mnt->fs->root})

#define	S_IFMT   0170000	/* type of file */
#define S_IFDIR  0040000	/* directory */
#define S_IFCHR  0020000	/* character special */
#define S_IFBLK  0060000	/* block special */
#define S_IFREG  0100000	/* regular */
#define S_IFLNK  0120000	/* symbolic link */
#define S_IFSOCK 0140000	/* socket */
#define S_IFIFO  0010000	/* fifo */

#define S_GETTTYPE(m) ((m) & S_IFMT)
#define	S_ISDIR(m)	(S_GETTTYPE(m) == S_IFDIR)
#define	S_ISCHR(m)	(S_GETTTYPE(m) == S_IFCHR)
#define	S_ISBLK(m)	(S_GETTTYPE(m) == S_IFBLK)
#define	S_ISREG(m)	(S_GETTTYPE(m) == S_IFREG)
#define	S_ISLNK(m)	(S_GETTTYPE(m) == S_IFLNK)
#define	S_ISSOCK(m)	(S_GETTTYPE(m) == S_IFSOCK)
#define	S_ISFIFO(m)	(S_GETTTYPE(m) == S_IFIFO)

#define	S_ISUID		0004000	/* set user id on execution */
#define	S_ISGID		0002000	/* set group id on execution */
#define	S_ISVTX		0001000	/* save swapped text even after use */
#define	S_IREAD		0000400	/* read permission, owner */
#define	S_IWRITE 	0000200	/* write permission, owner */
#define	S_IEXEC		0000100	/* execute/search permission, owner */
#define	S_ENFMT 	0002000	/* enforcement-mode locking */

typedef struct fs_type fs_type_t;
typedef struct fs fs_t;

typedef struct mount mount_t;

typedef struct path path_t;

typedef struct file file_t;
typedef struct dirent dirent_t;
typedef struct file_ops file_ops_t;

typedef struct inode inode_t;
typedef struct inode_ops inode_ops_t;

typedef struct dentry dentry_t;

typedef struct stat stat_t;

extern mount_t *root_mount;

#include "common/types.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "fs/fd.h"
#include "fs/block.h"

#define DENTRY_HASH_BITS 5

struct fs_type {
    const char *name;
    uint32_t flags;

    dentry_t * (*create)(fs_type_t *fs_type, const char *device);
    void (*destroy)(fs_t *fs);

    list_head_t instances;

    hashtable_node_t node;
};

struct fs {
    fs_type_t *type;
    void *private;
    dentry_t *root;

    list_head_t list;
};

struct mount {
    mount_t *parent;
    fs_t *fs;
    dentry_t *mountpoint;

    hashtable_node_t node;
};

struct path {
    mount_t *mount;
    dentry_t *dentry;
};

struct file {
    dentry_t *dentry;

    void *private;

    uint32_t offset;
    uint32_t refs;

    file_ops_t *ops;
};

#define ENTRY_TYPE_FILE 0
#define ENTRY_TYPE_DIR  1

typedef struct dir_entry_dat {
    uint32_t ino;
    uint8_t type;
    char name[256];
} dir_entry_dat_t;

struct file_ops {
    void (*open)(file_t *file, inode_t *inode);
    void (*close)(file_t *file);

    off_t (*seek)(file_t *file, off_t offset);
    ssize_t (*read)(file_t *file, char *buff, size_t bytes);
    ssize_t (*write) (file_t *file, const char *buff, size_t bytes);

    uint32_t (*iterate)(file_t *file, dir_entry_dat_t *buff, uint32_t num);
};

struct inode {
    fs_t *fs;
    inode_ops_t *ops;

    uint32_t flags;

    uint32_t dev;
    uint32_t ino;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint32_t rdev;
    uint32_t size;

    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;

    int32_t blkshift;
    int32_t blocks;

    void *private;
};

struct inode_ops {
    file_ops_t *file_ops;

    //Target is supposedly a child of inode. Populate target->inode with the
    //correct inode if it exists, or set target->inode = NULL if it does not.
    void (*lookup)(inode_t *inode, dentry_t *target);

    bool (*create)(inode_t *inode, dentry_t *d, uint32_t mode);
    bool (*mkdir)(inode_t *inode, dentry_t *d, uint32_t mode);
    void (*getattr)(dentry_t *dentry, stat_t *stat);
};

struct dentry {
    fs_t *fs;
    const char *name;
    uint32_t flags;

    inode_t *inode;

    dentry_t *parent;
    DECLARE_HASHTABLE(children_tab, DENTRY_HASH_BITS);
    list_head_t children_list;

    void *private;

    hashtable_node_t node;
    list_head_t list;
};

struct stat {
    int16_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    int16_t st_rdev;
    int32_t st_size;
    int64_t st_atime;
    int32_t st_spare1;
    int64_t st_mtime;
    int32_t st_spare2;
    int64_t st_ctime;
    int32_t st_spare3;
    int32_t st_blksize;
    int32_t st_blocks;
    int32_t st_spare4[2];
};

dentry_t * dentry_alloc(const char *name);
inode_t * inode_alloc(fs_t *fs, inode_ops_t *ops);
file_t * file_alloc(file_ops_t *ops);

void dentry_activate(dentry_t *child, dentry_t *parent);

void register_fs_type(fs_type_t *fs_type);

mount_t * vfs_mount(const char *raw_type, const char *device, path_t *mountpoint);
fs_t * vfs_fs_create(const char *raw_type, const char *device);
void vfs_fs_destroy(fs_t *fs);
mount_t * vfs_do_mount(fs_t *fs, path_t *mountpoint);
mount_t * vfs_mount_create(fs_t *fs);
bool vfs_mount_add(mount_t *mount, path_t *mountpoint);
void vfs_mount_destroy(mount_t *mount);

bool vfs_umount(path_t *mountpoint);

bool vfs_create(const path_t *start, const char *pathname, uint32_t mode, bool excl);
bool vfs_mkdir(const path_t *start, const char *pathname, uint32_t mode);

uint32_t simple_file_iterate(file_t *file, dir_entry_dat_t *buff, uint32_t num);

bool fs_no_create(inode_t *inode, dentry_t *d, uint32_t mode);
bool fs_no_mkdir(inode_t *inode, dentry_t *d, uint32_t mode);
dentry_t * fs_create_nodev(fs_type_t *type, void (*fill)(fs_t *fs));
dentry_t * fs_create_single(fs_type_t *type, void (*fill)(fs_t *fs));

void vfs_getattr(dentry_t *dentry, stat_t *stat);
void generic_getattr(inode_t *inode, stat_t *stat);

bool vfs_lookup(const path_t *start, const char *path, path_t *out);
file_t * vfs_open_file(dentry_t *dentry);
off_t vfs_seek(file_t *file, uint32_t off);
ssize_t vfs_read(file_t *file, void *buff, size_t bytes);
ssize_t vfs_write(file_t *file, const void *buff, size_t bytes);
uint32_t vfs_iterate(file_t *file, dir_entry_dat_t *buff, uint32_t num);

#endif
