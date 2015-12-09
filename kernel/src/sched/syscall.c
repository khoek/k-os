#include <stddef.h>

#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "bug/panic.h"
#include "mm/cache.h"
#include "time/timer.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "net/socket.h"
#include "fs/vfs.h"
#include "video/log.h"

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

typedef void (*syscall_t)(interrupt_t *);

static void sys_exit(interrupt_t *interrupt) {
    kprintf("sys_exit: %d", interrupt->cpu.reg.ecx);

    task_exit(current, interrupt->cpu.reg.ecx);
}

static void sys_fork(interrupt_t *interrupt) {
    current->ret = ~interrupt->cpu.reg.ecx;
}

static void wake_task(task_t *task) {
    task_wake(task);
}

static void sys_sleep(interrupt_t *interrupt) {
    task_sleep(current);
    timer_create(interrupt->cpu.reg.ecx, (void (*)(void *)) wake_task, current);
    sched_switch();
}

static void sys_log(interrupt_t *interrupt) {
    if(interrupt->cpu.reg.edx > 1023) {
        kprintf("syscall - task log string too long!");
    } else {
        char *buff = kmalloc(interrupt->cpu.reg.edx + 1);
        memcpy(buff, (void *) interrupt->cpu.reg.ecx, interrupt->cpu.reg.edx);
        buff[interrupt->cpu.reg.edx] = '\0';

        kprintf("user[%u] - %s", current->pid, buff);

        kfree(buff, interrupt->cpu.reg.edx + 1);
    }
}

static void sys_uptime(interrupt_t *interrupt) {
    current->ret = uptime();
}

static void sys_open(interrupt_t *interrupt) {
    //TODO verify that interrupt->cpu.reg.ecx is a pointer to a valid path string, and that interrupt->cpu.reg.edx are valid flags

    path_t path;
    if(!vfs_lookup(&current->pwd, (const char *) interrupt->cpu.reg.ecx, &path)) current->ret = -1;
    else {
        current->ret = ufdt_add(current, interrupt->cpu.reg.edx, vfs_open_file(path.dentry->inode));
    }
}

static void sys_close(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize buffer/size arguments

        gfdt_put(fd);

        current->ret = 0;
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_socket(interrupt_t *interrupt) {
    gfd_idx_t fd = sock_create_fd(sock_create(interrupt->cpu.reg.ecx, interrupt->cpu.reg.edx, interrupt->cpu.reg.ebx));

    current->ret = fd == FD_INVALID ? -1 : ufdt_add(current, 0, fd);
}

static void sys_listen(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        current->ret = sock_listen(gfd_to_sock(fd), interrupt->cpu.reg.edx > INT32_MAX ? 0 : interrupt->cpu.reg.edx) ? 0 : -1;
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_accept(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize address buff/size buff arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) interrupt->cpu.reg.edx;

        sock_t *child = sock_accept(sock);

        if(child) {
            current->ret = ufdt_add(current, 0, sock_create_fd(child));

            if(useraddr) {
                useraddr->sa_family = child->family->family;
                memcpy(child->peer.addr, &useraddr->sa_data, child->family->addr_len);
                *((socklen_t *) interrupt->cpu.reg.ebx) = child->family->addr_len;
            }
        } else {
            current->ret = -1;
        }
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_bind(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) interrupt->cpu.reg.edx;

        sock_addr_t addr;
        if(interrupt->cpu.reg.ebx < sock->family->addr_len + sizeof(struct sockaddr)) {
            //FIXME errno = EINVAL

            current->ret = -1;
        } else if(useraddr->sa_family != sock->family->family) {
            //FIXME errno = EAFNOSUPPORT

            current->ret = -1;
        } else {
            void *rawaddr = kmalloc(sock->family->addr_len);
            memcpy(rawaddr, &useraddr->sa_data, sock->family->addr_len);

            addr.family = sock->family->family;
            addr.addr = rawaddr;

            current->ret = sock_bind(sock, &addr) ? 0 : -1;
        }
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_connect(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) interrupt->cpu.reg.edx;

        sock_addr_t addr;
        if(useraddr->sa_family == AF_UNSPEC) {
            addr.family = AF_UNSPEC;
            addr.addr = NULL;

            current->ret = sock_connect(sock, &addr) ? 0 : -1;
        } else {
            if(interrupt->cpu.reg.ebx < sock->family->addr_len + sizeof(struct sockaddr)) {
                //FIXME errno = EINVAL

                current->ret = -1;
            } else if(useraddr->sa_family != sock->family->family) {
                //FIXME errno = EAFNOSUPPORT

                current->ret = -1;
            } else {
                void *rawaddr = kmalloc(sock->family->addr_len);
                memcpy(rawaddr, &useraddr->sa_data, sock->family->addr_len);

                addr.family = sock->family->family;
                addr.addr = rawaddr;

                current->ret = sock_connect(sock, &addr) ? 0 : -1;
            }
        }
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_shutdown(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        current->ret = sock_shutdown(gfd_to_sock(fd), interrupt->cpu.reg.edx);
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_send(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize buffer/size arguments

        //This buffer will be freed by the net subsystem
        void *buff = kmalloc(interrupt->cpu.reg.ebx);
        memcpy(buff, (void *) interrupt->cpu.reg.edx, interrupt->cpu.reg.ebx);

        current->ret = sock_send(gfd_to_sock(fd), buff, interrupt->cpu.reg.ebx, interrupt->cpu.reg.esi);
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_recv(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize buffer/size arguments

        current->ret = sock_recv(gfd_to_sock(fd), (void *) interrupt->cpu.reg.edx, interrupt->cpu.reg.ebx, interrupt->cpu.reg.esi);
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_alloc_page(interrupt_t *interrupt) {
    panic("Unimplemented");
}

static void sys_free_page(interrupt_t *interrupt) {
    panic("Unimplemented");
}

static void sys_stat(interrupt_t *interrupt) {
    //TODO verify that interrupt->cpu.reg.ecx is a pointer to a valid path string, and that interrupt->cpu.reg.edx are valid flags

    path_t path;
    if(!vfs_lookup(&current->pwd, (const char *) interrupt->cpu.reg.ecx, &path)) current->ret = -1;
    else {
        vfs_getattr(path.dentry, (void *) interrupt->cpu.reg.edx);

        current->ret = 0;
    }
}

static void sys_fstat(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize stat ptr

        vfs_getattr(gfdt_get(fd)->dentry, (void *) interrupt->cpu.reg.edx);

        current->ret = 0;
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

extern void play(uint32_t freq);
extern void stop();

static void sys_play(interrupt_t *interrupt) {
    play(interrupt->cpu.reg.ecx);
}

static void sys_stop(interrupt_t *interrupt) {
    stop();
}

static void sys_read(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize buffer/size arguments

        void *buff = kmalloc(interrupt->cpu.reg.ebx);
        memcpy(buff, (void *) interrupt->cpu.reg.edx, interrupt->cpu.reg.ebx);

        current->ret = vfs_read(gfdt_get(fd), buff, interrupt->cpu.reg.ebx);
        gfdt_put(fd);

        kfree(buff, interrupt->cpu.reg.ebx);
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
}

static void sys_write(interrupt_t *interrupt) {
    gfd_idx_t fd = ufdt_get(current, interrupt->cpu.reg.ecx);

    if(fd == FD_INVALID) current->ret = -1;
    else {
        //TODO sanitize buffer/size arguments

        current->ret = vfs_write(gfdt_get(fd), (void *) interrupt->cpu.reg.edx, interrupt->cpu.reg.ebx);
        gfdt_put(fd);
    }

    ufdt_put(current, interrupt->cpu.reg.ecx);
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
    if(interrupt->cpu.reg.eax >= MAX_SYSCALL || syscalls[interrupt->cpu.reg.eax] == NULL) {
        panicf("Unregistered Syscall #%u: 0x%X", interrupt->cpu.reg.eax, interrupt->cpu.reg.ecx);
    } else {
        syscalls[interrupt->cpu.reg.eax](interrupt);

        interrupt->cpu.reg.edx = current->ret >> 32;
        interrupt->cpu.reg.eax = current->ret;
    }
}

static INITCALL syscall_init() {
    register_isr(0x80, CPL_USER, syscall_handler, NULL);

    return 0;
}

core_initcall(syscall_init);
