#include <stddef.h>

#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/cache.h"
#include "time/timer.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "net/socket.h"
#include "fs/vfs.h"
#include "log/log.h"

#define MAX_SYSCALL 256

static inline void * param_ptr(interrupt_t *interrupt, uint32_t num) {
    return (void *) *((uint32_t *) (interrupt->cpu.exec.esp + (num * sizeof(uint32_t))));
}

static inline uint32_t param_32(interrupt_t *interrupt, uint32_t num) {
    return *((uint32_t *) (interrupt->cpu.exec.esp + (num * sizeof(uint32_t))));
}

static inline uint16_t param_16(interrupt_t *interrupt, uint32_t num) {
    return (uint16_t) *((uint32_t *) (interrupt->cpu.exec.esp + (num * sizeof(uint32_t))));
}

static inline uint8_t param_8(interrupt_t *interrupt, uint32_t num) {
    return (uint8_t) *((uint32_t *) (interrupt->cpu.exec.esp + (num * sizeof(uint32_t))));
}

typedef uint64_t (*syscall_t)(registers_t *);

static uint64_t sys_exit(registers_t *reg) {
    task_exit(current, reg->ecx);

    BUG();
    return 0;
}

static uint64_t sys_fork(registers_t *reg) {
    return ~reg->ecx;
}

static void wake_task(task_t *task) {
    task_wake(task);
}

static uint64_t sys_sleep(registers_t *reg) {
    task_sleep(current);
    timer_create(reg->ecx, (void (*)(void *)) wake_task, current);
    sched_switch();

    return 0;
}

static uint64_t sys_log(registers_t *reg) {
    if(reg->edx > 1023) {
        kprintf("syscall - task log string too long!");
    } else {
        char *buff = kmalloc(reg->edx + 1);
        memcpy(buff, (void *) reg->ecx, reg->edx);
        buff[reg->edx] = '\0';

        kprintf("user[%u] - %s", current->pid, buff);

        kfree(buff, reg->edx + 1);
    }

    return 0;
}

static uint64_t sys_uptime(registers_t *reg) {
    return uptime();
}

static uint64_t sys_open(registers_t *reg) {
    //TODO verify that reg->ecx is a pointer to a valid path string, and that reg->edx are valid flags

    path_t path;
    if(vfs_lookup(&current->pwd, (const char *) reg->ecx, &path)) {
        return ufdt_add(current, reg->edx, vfs_open_file(path.dentry->inode));
    } else {
        return -1;
    }
}

static uint64_t sys_close(registers_t *reg) {
    ufd_idx_t ufd = reg->ecx;
    //TODO sanitize arguments

    file_t *gfd = ufdt_get(current, ufd);
    if(gfd) {
        ufdt_close(current, ufd);
        ufdt_put(current, ufd);

        return 0;
    } else {
        return -1;
    }
}

static uint64_t sys_socket(registers_t *reg) {
    int32_t ret = -1;

    sock_t *sock = sock_create(reg->ecx, reg->edx, reg->ebx);
    if(sock) {
        file_t *fd = sock_create_fd(sock);
        if(fd) {
            ret = ufdt_add(current, 0, fd);
            if(ret == -1) {
                //TODO free sock and fd
            }
        }
    }

    return ret;
}

static uint64_t sys_listen(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        ret = sock_listen(gfd_to_sock(fd), reg->edx) ? 0 : -1;
    }

    ufdt_put(current, reg->ecx);

    return ret;
}

static uint64_t sys_accept(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize address buff/size buff arguments

        sock_t *sock = gfd_to_sock(fd);
        struct sockaddr *useraddr = (struct sockaddr *) reg->edx;
        sock_t *child = sock_accept(sock);

        if(child) {
            ret = ufdt_add(current, 0, sock_create_fd(child));

            if(useraddr) {
                useraddr->sa_family = child->family->family;
                memcpy(child->peer.addr, &useraddr->sa_data, child->family->addr_len);
                *((socklen_t *) reg->ebx) = child->family->addr_len;
            }
        }

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_bind(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) reg->edx;

        sock_addr_t addr;
        if(reg->ebx < sock->family->addr_len + sizeof(struct sockaddr)) {
            //FIXME errno = EINVAL
        } else if(useraddr->sa_family != sock->family->family) {
            //FIXME errno = EAFNOSUPPORT
        } else {
            void *rawaddr = kmalloc(sock->family->addr_len);
            memcpy(rawaddr, &useraddr->sa_data, sock->family->addr_len);

            addr.family = sock->family->family;
            addr.addr = rawaddr;

            ret = sock_bind(sock, &addr) ? 0 : -1;
        }

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_connect(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) reg->edx;

        sock_addr_t addr;
        if(useraddr->sa_family == AF_UNSPEC) {
            addr.family = AF_UNSPEC;
            addr.addr = NULL;

            ret = sock_connect(sock, &addr) ? 0 : -1;
        } else {
            if(reg->ebx < sock->family->addr_len + sizeof(struct sockaddr)) {
                //FIXME errno = EINVAL
            } else if(useraddr->sa_family != sock->family->family) {
                //FIXME errno = EAFNOSUPPORT
            } else {
                void *rawaddr = kmalloc(sock->family->addr_len);
                memcpy(rawaddr, &useraddr->sa_data, sock->family->addr_len);

                addr.family = sock->family->family;
                addr.addr = rawaddr;

                ret = sock_connect(sock, &addr) ? 0 : -1;
            }
        }

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_shutdown(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        ret = sock_shutdown(gfd_to_sock(fd), reg->edx);

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_send(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize buffer/size arguments

        //This buffer will be freed by the net subsystem
        void *buff = kmalloc(reg->ebx);
        memcpy(buff, (void *) reg->edx, reg->ebx);

        ret = sock_send(gfd_to_sock(fd), buff, reg->ebx, reg->esi);

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_recv(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = sock_recv(gfd_to_sock(fd), (void *) reg->edx, reg->ebx, reg->esi);

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_alloc_page(registers_t *reg) {
    panic("Unimplemented");
}

static uint64_t sys_free_page(registers_t *reg) {
    panic("Unimplemented");
}

static uint64_t sys_stat(registers_t *reg) {
    int32_t ret = -1;

    //TODO verify that reg->ecx is a pointer to a valid path string, and that reg->edx are valid flags

    path_t path;
    if(vfs_lookup(&current->pwd, (const char *) reg->ecx, &path)) {
        vfs_getattr(path.dentry, (void *) reg->edx);

        ret = 0;
    }

    return ret;
}

static uint64_t sys_fstat(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize stat ptr

        vfs_getattr(fd->dentry, (void *) reg->edx);

        ret = 0;

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

extern void play(uint32_t freq);
extern void stop();

static uint64_t sys_play(registers_t *reg) {
    play(reg->ecx);

    return 0;
}

static uint64_t sys_stop(registers_t *reg) {
    stop();

    return 0;
}

static uint64_t sys_read(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize buffer/size arguments

        void *buff = kmalloc(reg->ebx);
        memcpy(buff, (void *) reg->edx, reg->ebx);

        ret = vfs_read(fd, buff, reg->ebx);

        kfree(buff, reg->ebx);

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static uint64_t sys_write(registers_t *reg) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(current, reg->ecx);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_write(fd, (void *) reg->edx, reg->ebx);

        ufdt_put(current, reg->ecx);
    }

    return ret;
}

static syscall_t syscalls[MAX_SYSCALL] = {
    [ 0] = sys_exit,
    [ 1] = sys_fork,
    [ 2] = sys_sleep,
    [ 3] = sys_log,
    [ 4] = sys_uptime,
    [ 5] = sys_open,
    [ 6] = sys_close,
    [ 7] = sys_socket,
    [ 8] = sys_listen,
    [ 9] = sys_accept,
    [10] = sys_bind,
    [11] = sys_connect,
    [12] = sys_shutdown,
    [13] = sys_send,
    [14] = sys_recv,
    [15] = sys_alloc_page,
    [16] = sys_free_page,
    [17] = sys_stat,
    [19] = sys_fstat,
    [20] = sys_play,
    [21] = sys_stop,
    [22] = sys_read,
    [23] = sys_write,
};

static void syscall_handler(interrupt_t *interrupt, void *data) {
    registers_t *reg = &interrupt->cpu.reg;
    if(reg->eax >= MAX_SYSCALL || !syscalls[reg->eax]) {
        panicf("Unregistered Syscall #%u: 0x%X", reg->eax, reg->ecx);
    } else {
        uint64_t ret = syscalls[reg->eax](reg);
        reg->edx = ret >> 32;
        reg->eax = ret;
    }
}

static INITCALL syscall_init() {
    register_isr(0x80, CPL_USER, syscall_handler, NULL);

    return 0;
}

core_initcall(syscall_init);
