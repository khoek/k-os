#include "common/types.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/pl.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/cache.h"
#include "time/timer.h"
#include "time/clock.h"
#include "sync/atomic.h"
#include "sched/task.h"
#include "sched/sched.h"
#include "net/socket.h"
#include "fs/vfs.h"
#include "fs/exec.h"
#include "log/log.h"

#define MAX_SYSCALL 256

typedef uint64_t (*syscall_t)(cpu_state_t *);

#define SYSCALL_NAME(name) sys_##name
#define DEFINE_SYSCALL(name) uint64_t SYSCALL_NAME(name) (cpu_state_t *state)
#define reg(r) (state->reg.r)

static DEFINE_SYSCALL(exit) {
    task_node_exit(reg(ecx));

    //We should have S_DIE marked right now, which will kill us in
    //deliver_signals().

    return 0;
}

typedef struct fork_data {
    cpu_state_t resume_state;
} fork_data_t;

void ret_from_fork(void *arg) {
    check_irqs_disabled();

    fork_data_t forkd;
    memcpy(&forkd, arg, sizeof(fork_data_t));
    kfree(arg);

    forkd.resume_state.reg.edx = 0;
    forkd.resume_state.reg.eax = 0;

    pl_enter_userland((void *) forkd.resume_state.exec.eip, (void *) forkd.resume_state.stack.esp, &forkd.resume_state.reg);
}

static DEFINE_SYSCALL(fork) {
    fork_data_t *forkd = kmalloc(sizeof(fork_data_t));
    memcpy(&forkd->resume_state, state, sizeof(cpu_state_t));
    pid_t cpid = thread_fork(current, 0, ret_from_fork, forkd);

    return cpid;
}

typedef struct sleep_data {
    thread_t *task;
    spinlock_t lock;
} sleep_data_t;

static void sleep_callback(sleep_data_t *data) {
    spin_lock(&data->lock);
    spin_unlock(&data->lock);

    thread_wake(data->task);
}

static DEFINE_SYSCALL(msleep) {
    sleep_data_t data;
    data.task = current;
    spinlock_init(&data.lock);

    uint32_t flags;
    spin_lock_irqsave(&data.lock, &flags);

    timer_create(reg(ecx)/100, (void (*)(void *)) sleep_callback, &data);
    thread_sleep_prepare();

    spin_unlock_irqstore(&data.lock, flags);

    sched_switch();

    return 0;
}

static DEFINE_SYSCALL(log) {
    if(reg(edx) > 1023) {
        kprintf("syscall - task log string too long!");
    } else {
        char *buff = kmalloc(reg(edx) + 1);
        memcpy(buff, (void *) reg(ecx), reg(edx));
        buff[reg(edx)] = '\0';

        kprintf("user[%u] - %s", obtain_task_node(current)->pid, buff);

        kfree(buff);
    }

    return 0;
}

static DEFINE_SYSCALL(uptime) {
    return uptime();
}

static DEFINE_SYSCALL(open) {
    //TODO verify that reg(ecx) is a pointer to a valid path string, and that reg(edx) are valid flags

    path_t pwd = get_pwd(current);

    path_t path;
    if(vfs_lookup(&pwd, (const char *) reg(ecx), &path)) {
        return ufdt_add(reg(edx), vfs_open_file(path.dentry->inode));
    } else {
        return -1;
    }
}

static DEFINE_SYSCALL(close) {
    ufd_idx_t ufd = reg(ecx);
    //TODO sanitize arguments

    file_t *gfd = ufdt_get(ufd);
    if(gfd) {
        ufdt_close(ufd);
        ufdt_put(ufd);

        return 0;
    } else {
        return -1;
    }
}

static DEFINE_SYSCALL(socket) {
    int32_t ret = -1;

    sock_t *sock = sock_create(reg(ecx), reg(edx), reg(ebx));
    if(sock) {
        file_t *fd = sock_create_fd(sock);
        if(fd) {
            ret = ufdt_add(0, fd);
            if(ret == -1) {
                //TODO free sock and fd
            }
        }
    }

    return ret;
}

static DEFINE_SYSCALL(listen) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        ret = sock_listen(gfd_to_sock(fd), reg(edx)) ? 0 : -1;
    }

    ufdt_put(reg(ecx));

    return ret;
}

static DEFINE_SYSCALL(accept) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize address buff/size buff arguments

        sock_t *sock = gfd_to_sock(fd);
        struct sockaddr *useraddr = (struct sockaddr *) reg(edx);
        sock_t *child = sock_accept(sock);

        if(child) {
            ret = ufdt_add(0, sock_create_fd(child));

            if(useraddr) {
                useraddr->sa_family = child->family->family;
                memcpy(child->peer.addr, &useraddr->sa_data, child->family->addr_len);
                *((socklen_t *) reg(ebx)) = child->family->addr_len;
            }
        }

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(bind) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) reg(edx);

        sock_addr_t addr;
        if(reg(ebx) < sock->family->addr_len + sizeof(struct sockaddr)) {
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

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(connect) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        struct sockaddr *useraddr = (struct sockaddr *) reg(edx);

        sock_addr_t addr;
        if(useraddr->sa_family == AF_UNSPEC) {
            addr.family = AF_UNSPEC;
            addr.addr = NULL;

            ret = sock_connect(sock, &addr) ? 0 : -1;
        } else {
            if(reg(ebx) < sock->family->addr_len + sizeof(struct sockaddr)) {
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

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(shutdown) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        ret = sock_shutdown(gfd_to_sock(fd), reg(edx));

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(send) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize buffer/size arguments

        //This buffer will be freed by the net subsystem
        void *buff = kmalloc(reg(ebx));
        memcpy(buff, (void *) reg(edx), reg(ebx));

        ret = sock_send(gfd_to_sock(fd), buff, reg(ebx), reg(esi));

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(recv) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = sock_recv(gfd_to_sock(fd), (void *) reg(edx), reg(ebx), reg(esi));

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(alloc_page) {
    panic("Unimplemented");
}

static DEFINE_SYSCALL(free_page) {
    panic("Unimplemented");
}

static DEFINE_SYSCALL(stat) {
    int32_t ret = -1;

    //TODO verify that reg(ecx) is a pointer to a valid path string, and that reg(edx) are valid flags

    thread_t *me = current;

    path_t pwd;
    uint32_t flags;
    spin_lock_irqsave(&me->fs->lock, &flags);
    pwd = me->fs->pwd;
    spin_unlock_irqstore(&me->fs->lock, flags);

    path_t path;
    if(vfs_lookup(&pwd, (const char *) reg(ecx), &path)) {
        vfs_getattr(path.dentry, (void *) reg(edx));

        ret = 0;
    }

    return ret;
}

static DEFINE_SYSCALL(fstat) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize stat ptr

        vfs_getattr(fd->dentry, (void *) reg(edx));

        ret = 0;

        ufdt_put(reg(ecx));
    }

    return ret;
}

extern void play(uint32_t freq);
extern void stop();

static DEFINE_SYSCALL(play) {
    play(reg(ecx));

    return 0;
}

static DEFINE_SYSCALL(stop) {
    stop();

    return 0;
}

static DEFINE_SYSCALL(read) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize buffer/size arguments

        void *buff = kmalloc(reg(ebx));
        ret = vfs_read(fd, buff, reg(ebx));
        memcpy((void *) reg(edx), buff, reg(ebx));
        kfree(buff);

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(write) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(reg(ecx));
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_write(fd, (void *) reg(edx), reg(ebx));

        ufdt_put(reg(ecx));
    }

    return ret;
}

static DEFINE_SYSCALL(execve) {
    char *pathname = (void *) reg(ecx);

    //FIXME sanitize
    char **argv = copy_strtab((void *) reg(edx));
    char **envp = copy_strtab((void *) reg(ebx));

    thread_t *me = current;

    path_t pwd;
    uint32_t flags;
    spin_lock_irqsave(&me->fs->lock, &flags);
    pwd = me->fs->pwd;
    spin_unlock_irqstore(&me->fs->lock, flags);

    path_t p;
    if(!vfs_lookup(&pwd, pathname, &p)) {
        return -1;
    }

    bool ret = execute_path(&p, argv, envp);
    BUG_ON(ret);

    return -1;
}

static task_node_t * reap_zombie(task_node_t *node) {
    task_node_t *zombie = NULL;

    uint32_t flags;
    spin_lock_irqsave(&node->lock, &flags);

    if(!list_empty(&node->zombies)) {
        zombie = list_first(&node->zombies, task_node_t, zombie_list);
        list_rm(&zombie->zombie_list);
    }

    spin_unlock_irqstore(&node->lock, flags);

    return zombie;
}

#define WSTATE_EXITED (1 << 8)

static task_node_t * find_child(task_node_t *me, pid_t pid) {
    task_node_t *o;
    LIST_FOR_EACH_ENTRY(o, &me->children, child_list) {
        if(o->pid == pid) {
            return o;
        }
    }

    return NULL;
}

static DEFINE_SYSCALL(waitpid) {
    thread_t *me = current;

    pid_t pid = (pid_t) reg(ecx);
    int *stat_loc = (void *) reg(edx);
    int UNUSED(options) = (int) reg(ebx);

    task_node_t *node = obtain_task_node(me);

    pid_t cpid = -1;
    if(pid == -1) {
        task_node_t *zombie;
        if(!wait_for_condition(zombie = reap_zombie(node))) {
            return -1; //FIXME ERRINTR
        }

        if(stat_loc) *stat_loc = WSTATE_EXITED | zombie->exit_code;
        cpid = zombie->pid;
    } else if (pid > 0) {
        task_node_t *child = find_child(node, pid);

        BUG_ON(!child);

        if(!wait_for_condition(atomic_read(&child->exit_state) == TASK_EXITED)) {
            return -1; //FIXME ERRINTR
        }

        cpid = pid;
    } else {
        UNIMPLEMENTED();
    }

    return cpid;
}

static syscall_t syscalls[MAX_SYSCALL] = {
    [ 0] = SYSCALL_NAME(exit),
    [ 1] = SYSCALL_NAME(fork),
    [ 2] = SYSCALL_NAME(msleep),
    [ 3] = SYSCALL_NAME(log),
    [ 4] = SYSCALL_NAME(uptime),
    [ 5] = SYSCALL_NAME(open),
    [ 6] = SYSCALL_NAME(close),
    [ 7] = SYSCALL_NAME(socket),
    [ 8] = SYSCALL_NAME(listen),
    [ 9] = SYSCALL_NAME(accept),
    [10] = SYSCALL_NAME(bind),
    [11] = SYSCALL_NAME(connect),
    [12] = SYSCALL_NAME(shutdown),
    [13] = SYSCALL_NAME(send),
    [14] = SYSCALL_NAME(recv),
    [15] = SYSCALL_NAME(alloc_page),
    [16] = SYSCALL_NAME(free_page),
    [17] = SYSCALL_NAME(stat),
    [19] = SYSCALL_NAME(fstat),
    [20] = SYSCALL_NAME(play),
    [21] = SYSCALL_NAME(stop),
    [22] = SYSCALL_NAME(read),
    [23] = SYSCALL_NAME(write),
    [24] = SYSCALL_NAME(execve),
    [25] = SYSCALL_NAME(waitpid),
};

static void syscall_handler(interrupt_t *interrupt, void *data) {
    enter_syscall();

    cpu_state_t *state = &interrupt->cpu;

    if(state->reg.eax >= MAX_SYSCALL || !syscalls[state->reg.eax]) {
        panicf("Unregistered Syscall #%u", state->reg.eax);
    } else {
        uint64_t ret = syscalls[state->reg.eax](state);
        state->reg.edx = ret >> 32;
        state->reg.eax = ret;
    }

    leave_syscall();

    //This might not return (as in S_DIE).
    deliver_signals();
}

static INITCALL syscall_init() {
    register_isr(0x80, CPL_USER, syscall_handler, NULL);

    return 0;
}

core_initcall(syscall_init);
