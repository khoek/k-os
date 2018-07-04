#include "common/types.h"
#include "lib/string.h"
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
#include "sched/syscall.h"
#include "net/socket.h"
#include "fs/vfs.h"
#include "fs/exec.h"
#include "log/log.h"

syscall_t syscalls[MAX_SYSCALL] = {
#include "shared/syscall_ents.h"
};

DEFINE_SYSCALL(_exit, int32_t code) {
    task_node_exit(code);

    //We should have S_DIE marked right now, which will kill us in
    //deliver_signals().

    return 0;
}

DEFINE_SYSCALL(fork)  {
    void *prep = arch_prepare_fork(state);
    thread_t *child = thread_fork(current, 0, arch_ret_from_fork, prep);

    return child->node->pid;
}

static void sleep_callback(thread_t *task) {
    thread_wake(task);
}

DEFINE_SYSCALL(msleep, uint32_t millis) {
    irqdisable();

    thread_sleep_prepare();
    timer_create(millis, (void (*)(void *)) sleep_callback, current);
    sched_switch();

    return 0;
}

DEFINE_SYSCALL(log, const char *loc, uint32_t len) {
    if(len > 1023) {
        kprintf("syscall - task log string too long!");
    } else {
        char *buff = kmalloc(len + 1);
        memcpy(buff, loc, len);
        buff[len] = '\0';

        kprintf("user[%u] - %s", obtain_task_node(current)->pid, buff);

        kfree(buff);
    }

    return 0;
}

DEFINE_SYSCALL(uptime)  {
    return uptime();
}

DEFINE_SYSCALL(open, const char *pathname, uint32_t flags) {
    //TODO verify that path is a pointer to a valid path string, and that flags are valid flags

    if(!pathname) {
        //errno = EFAULT            <- I looked it up!/ (tried lon linux!)
        return -1;
    }

    path_t pwd = get_pwd(current);

    path_t path;
    if(vfs_lookup(&pwd, pathname, &path)) {
        return ufdt_add(flags, vfs_open_file(path.dentry));
    } else {
        return -1;
    }
}

DEFINE_SYSCALL(close, ufd_idx_t ufd) {
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

DEFINE_SYSCALL(socket, uint32_t family, uint32_t type, uint32_t protocol) {
    int32_t ret = -1;

    sock_t *sock = sock_create(family, type, protocol);
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

DEFINE_SYSCALL(listen, ufd_idx_t ufd, uint32_t backlog) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        ret = sock_listen(gfd_to_sock(fd), backlog) ? 0 : -1;
    }

    ufdt_put(ufd);

    return ret;
}

DEFINE_SYSCALL(accept, ufd_idx_t ufd, struct sockaddr *user_addr, socklen_t *len) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize address buff/size buff arguments

        sock_t *sock = gfd_to_sock(fd);
        sock_t *child = sock_accept(sock);

        if(child) {
            ret = ufdt_add(0, sock_create_fd(child));

            if(user_addr) {
                user_addr->sa_family = child->family->family;
                memcpy(child->peer.addr, &user_addr->sa_data, child->family->addr_len);
                *len = child->family->addr_len;
            }
        }

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(bind, ufd_idx_t ufd, const struct sockaddr *user_addr, socklen_t len) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        sock_addr_t addr;
        if(len < sock->family->addr_len + sizeof(struct sockaddr)) {
            //FIXME errno = EINVAL
        } else if(user_addr->sa_family != sock->family->family) {
            //FIXME errno = EAFNOSUPPORT
        } else {
            void *rawaddr = kmalloc(sock->family->addr_len);
            memcpy(rawaddr, &user_addr->sa_data, sock->family->addr_len);

            addr.family = sock->family->family;
            addr.addr = rawaddr;

            ret = sock_bind(sock, &addr) ? 0 : -1;
        }

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(connect, ufd_idx_t ufd, const struct sockaddr *user_addr, socklen_t len) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize address/size arguments

        sock_t *sock = gfd_to_sock(fd);

        sock_addr_t addr;
        if(user_addr->sa_family == AF_UNSPEC) {
            addr.family = AF_UNSPEC;
            addr.addr = NULL;

            ret = sock_connect(sock, &addr) ? 0 : -1;
        } else {
            if(len < sock->family->addr_len + sizeof(struct sockaddr)) {
                //FIXME errno = EINVAL
            } else if(user_addr->sa_family != sock->family->family) {
                //FIXME errno = EAFNOSUPPORT
            } else {
                void *rawaddr = kmalloc(sock->family->addr_len);
                memcpy(rawaddr, &user_addr->sa_data, sock->family->addr_len);

                addr.family = sock->family->family;
                addr.addr = rawaddr;

                ret = sock_connect(sock, &addr) ? 0 : -1;
            }
        }

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(shutdown, ufd_idx_t ufd, int how) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        ret = sock_shutdown(gfd_to_sock(fd), how);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(send, ufd_idx_t ufd, const void *user_buff, uint32_t buffsize, uint32_t flags) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        //This buffer will be freed by the net subsystem
        void *buff = kmalloc(buffsize);
        memcpy(buff, user_buff, buffsize);

        ret = sock_send(gfd_to_sock(fd), buff, buffsize, flags);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(recv, ufd_idx_t ufd, void *user_buff, uint32_t buffsize, uint32_t flags) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = sock_recv(gfd_to_sock(fd), user_buff, buffsize, flags);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(alloc_page, void *addr)  {
    page_t *page = alloc_page(0);
    user_map_page(current, addr, page_to_phys(page));
    return 0;
}

DEFINE_SYSCALL(free_page)  {
    UNIMPLEMENTED();
}

struct dirent {
    ino_t d_ino;
    off_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];		/* We must not include limits.h! */
} PACKED;

DEFINE_SYSCALL(getdents, ufd_idx_t ufd, struct dirent *user_buff, uint32_t buffsize) {
    int32_t ret = -1;
    uint32_t count = buffsize / sizeof(struct dirent);

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        dir_entry_dat_t *buff = kmalloc(count * sizeof(dir_entry_dat_t));
        uint32_t num = vfs_iterate(fd, buff, count);
        for(uint32_t i = 0; i < num; i++) {
          user_buff[i].d_ino = buff[i].ino;
          user_buff[i].d_off = sizeof(struct dirent);
          user_buff[i].d_reclen = sizeof(struct dirent);
          user_buff[i].d_type = buff[i].type;
          strcpy(user_buff[i].d_name, buff[i].name);
        }
        kfree(buff);

        ret = num;

        ufdt_put(ufd);
    }

    return ret * sizeof(struct dirent);
}

DEFINE_SYSCALL(stat, const char *pathname, void *buff) {
    int32_t ret = -1;

    //TODO verify that path is a pointer to a valid path string, etc.

    path_t pwd = get_pwd(current);

    path_t path;
    if(vfs_lookup(&pwd, pathname, &path)) {
        vfs_getattr(path.dentry, buff);

        ret = 0;
    }

    return ret;
}

DEFINE_SYSCALL(lstat)  {
    UNIMPLEMENTED();
}

DEFINE_SYSCALL(fstat, ufd_idx_t ufd, void *buff) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize stat ptr

        vfs_getattr(fd->dentry, buff);
        ret = 0;

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(read, ufd_idx_t ufd, void *user_buff, uint32_t len) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        void *buff = kmalloc(len);
        ret = vfs_read(fd, buff, len);
        memcpy(user_buff, buff, ret);
        kfree(buff);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(write, ufd_idx_t ufd, const void *buff, uint32_t len) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_write(fd, buff, len);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(fexecve, ufd_idx_t ufd, char *const user_argv[], char *const user_envp[]) {
    int32_t ret = -1;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //FIXME sanitize
        char **argv = copy_strtab(user_argv);
        char **envp = copy_strtab(user_envp);

        ret = execute_path(fd->dentry, argv, envp);
        BUG_ON(ret);
    }

    return ret;
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

DEFINE_SYSCALL(waitpid, pid_t pid, int *stat_loc, int UNUSED(options)) {
    task_node_t *node = obtain_task_node(current);

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

DEFINE_SYSCALL(unimplemented, char *msg) {
    if(!strcmp(msg, "fstat")) {
        return 0;
    }
    panicf("unimplemented libc hook: %s", msg);
}
