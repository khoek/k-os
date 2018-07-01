#ifndef LIBC_SYS_TYPES_H
#define LIBC_SYS_TYPES_H

#include <stddef.h>
#include <inttypes.h>

typedef int32_t ssize_t;

typedef uint32_t dev_t;
typedef uint32_t id_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t ino_t;
typedef uint32_t mode_t;
typedef uint32_t nlink_t;
typedef uint32_t clock_t;
typedef uint32_t time_t;

typedef int32_t off_t;
typedef int32_t pid_t;

typedef int32_t blkcnt_t;
typedef int32_t blksize_t;

typedef uint32_t useconds_t;
typedef int32_t suseconds_t;

typedef uint32_t ufd_idx_t;

#endif
