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
#include "driver/console/tty.h"
#include "log/log.h"
#include "user/select.h"
#include "user/wait.h"

syscall_t syscalls[MAX_SYSCALL] = {
#include "shared/syscall_ents.h"
};

DEFINE_SYSCALL(_exit, uint32_t code) {
    task_node_exit(code, ECAUSE_RQST);

    //We should have SIGKILL marked right now, which will kill us in
    //sched_deliver_signals().

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

DEFINE_SYSCALL(open, const char *pathname, uint32_t flags, uint32_t mode) {
    int32_t ret = 0;

    //FIXME actually mask this properly
    mode &= 0777;

    if(!pathname) {
        return -EFAULT;
    }

    //TODO verify that path is a pointer to a valid path string, and that flags are valid flags

    path_t *pwd = &obtain_fs_context(current)->pwd;

    path_t path;
    if(flags & O_CREAT) {
        ret = vfs_create(pwd, pathname, S_IFREG | mode, &path);
        if(ret == -EEXIST && !(flags & O_EXCL)) {
            ret = 0;
        }
    } else {
        ret = vfs_lookup(pwd, pathname, &path);
    }

    if(!ret) {
        file_t *file = vfs_open_file(&path);
        if(!file) {
            BUG();
        }
        return ufdt_add(file);
    }

    return ret;
}

DEFINE_SYSCALL(close, ufd_idx_t ufd) {
    //TODO sanitize arguments

    file_t *file = ufdt_get(ufd);
    if(file) {
        ufdt_close(ufd);
        ufdt_put(ufd);

        return 0;
    }

    return -EINVAL;
}

static void read_in_fdset(fd_set *kern, fd_set *user) {
    if(user) {
        *kern = *((fd_set *) user);
    } else {
        FD_ZERO(&kern);
    }
}

DEFINE_SYSCALL(select, int nfds, void *rfds, void *wfds, void *efds, struct timeval *timeout) {
    uint64_t then = uptime(); //in millis
    //FIXME precision
    uint64_t waittime = timeout ? (timeout->tv_sec * MILLIS_PER_SEC)
        + DIV_UP(timeout->tv_usec, MICROS_PER_MILLI) : 0;

    fd_set rfds_in, rfds_out;
    fd_set wfds_in, wfds_out;
    fd_set efds_in, efds_out;

    read_in_fdset(&rfds_in, rfds);
    read_in_fdset(&wfds_in, wfds);
    read_in_fdset(&efds_in, efds);
    FD_ZERO(&rfds_out);
    FD_ZERO(&wfds_out);
    FD_ZERO(&efds_out);

    uint32_t num = 0;
    while(!num) {
        for(uint32_t i = 0; i < (nfds < 0 ? 0 : nfds); i++) {
            file_t *fd = ufdt_get(i);
            if(!fd) continue;

            fpoll_data_t fp;
            int32_t ret = vfs_poll(fd, &fp);
            if(ret < 0) {
                panicf("poll failure! %d", ret);
            }

            if(FD_ISSET(i, &rfds_in) && fp.readable) {
                FD_ISSET(i, &rfds_out);
                num++;
            }

            if(FD_ISSET(i, &wfds_in) && fp.writable) {
                FD_ISSET(i, &wfds_out);
                num++;
            }

            if(FD_ISSET(i, &efds_in) && fp.errored) {
                FD_ISSET(i, &efds_out);
                num++;
            }
        }

        if(!num) {
            if(timeout && uptime() - then >= waittime) {
                break;
            }

            if(are_signals_pending(current)) {
                return -EINTR;
            }
            //FIXME need to set an alarm to make sure we get worken?
            sched_suspend_pending_interrupt();
        }
    }

    if(rfds) *((fd_set *) rfds) = rfds_out;
    if(wfds) *((fd_set *) wfds) = wfds_out;
    if(efds) *((fd_set *) efds) = efds_out;

    return -1;
}

DEFINE_SYSCALL(socket, uint32_t family, uint32_t type, uint32_t protocol) {
    int32_t ret = -EBADF;

    sock_t *sock = sock_create(family, type, protocol);
    if(sock) {
        file_t *fd = sock_create_fd(sock);
        if(fd) {
            ret = ufdt_add(fd);
            if(ret == -1) {
                //TODO free sock and fd
            }
        }
    }

    return ret;
}

DEFINE_SYSCALL(listen, ufd_idx_t ufd, uint32_t backlog) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        ret = sock_listen(gfd_to_sock(fd), backlog) ? 0 : -1;
    }

    ufdt_put(ufd);

    return ret;
}

DEFINE_SYSCALL(accept, ufd_idx_t ufd, struct sockaddr *user_addr, socklen_t *len) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize address buff/size buff arguments

        sock_t *sock = gfd_to_sock(fd);
        sock_t *child = sock_accept(sock);

        if(IS_ERR(child)) {
            ret = PTR_ERR(child);
        } else {
            ret = ufdt_add(sock_create_fd(child));

            if(user_addr) {
                user_addr->sa_family = child->family->family;
                memcpy(child->peer.addr, &user_addr->sa_data, child->family->addr_len);
                *len = child->family->addr_len;
            }
        }
    }

    return ret;
}

DEFINE_SYSCALL(bind, ufd_idx_t ufd, const struct sockaddr *user_addr, socklen_t len) {
    int32_t ret = -EBADF;

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
    int32_t ret = -EBADF;

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
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        ret = sock_shutdown(gfd_to_sock(fd), how);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(send, ufd_idx_t ufd, const void *user_buff, uint32_t buffsize, uint32_t flags) {
    int32_t ret = -EBADF;

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
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = sock_recv(gfd_to_sock(fd), user_buff, buffsize, flags);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(alloc_page, uint32_t pidx)  {
    page_t *page = alloc_page(0);
    user_map_page(current, (void *) (pidx * PAGE_SIZE), page_to_phys(page));
    return 0;
}

DEFINE_SYSCALL(free_page, uint32_t page)  {
    //FIXME implement me
    return 0;
}

struct dirent {
    ino_t d_ino;
    off_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];		/* We must not include limits.h! */
} PACKED;

DEFINE_SYSCALL(getdents, ufd_idx_t ufd, struct dirent *user_buff, uint32_t buffsize) {
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

        ufdt_put(ufd);

        return num * sizeof(struct dirent);
    }

    return -EINVAL;
}

DEFINE_SYSCALL(stat, const char *pathname, void *buff) {
    //FIXME implement symbolic links and make this different to lstat

    //TODO verify that path is a pointer to a valid path string, etc.

    path_t path;
    int32_t ret = vfs_lookup(&obtain_fs_context(current)->pwd, pathname, &path);
    if(!ret) {
        vfs_getattr(path.dentry, buff);
    }

    return ret;
}

DEFINE_SYSCALL(lstat, const char *pathname, void *buff)  {
    //TODO verify that path is a pointer to a valid path string, etc.

    path_t path;
    int32_t ret = vfs_lookup(&obtain_fs_context(current)->pwd, pathname, &path);
    if(!ret) {
        vfs_getattr(path.dentry, buff);
    }

    return ret;
}

DEFINE_SYSCALL(fstat, ufd_idx_t ufd, void *buff) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize stat ptr

        vfs_getattr(fd->path.dentry, buff);
        ret = 0;

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(read, ufd_idx_t ufd, void *user_buff, uint32_t len) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_read(fd, user_buff, len);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(write, ufd_idx_t ufd, const void *buff, uint32_t len) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_write(fd, buff, len);

        ufdt_put(ufd);
    }

    return ret;
}

static int32_t do_execve(path_t *path, char *const user_argv[],
    char *const user_envp[]) {
    //FIXME sanitize
    char **argv = copy_strtab(user_argv);
    char **envp = copy_strtab(user_envp);

    int32_t ret = execute_path(path, argv, envp);

    kfree(argv);
    kfree(envp);

    return ret;
}

DEFINE_SYSCALL(execve, const char *pathname, char *const user_argv[], char *const user_envp[]) {
    //FIXME sanitize pathname...
    path_t path;
    int32_t ret = vfs_lookup(&obtain_fs_context(current)->pwd, pathname, &path);
    if(ret) {
        return ret;
    }

    return do_execve(&path, user_argv, user_envp);
}

DEFINE_SYSCALL(fexecve, ufd_idx_t ufd, char *const user_argv[], char *const user_envp[]) {
    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        ret = do_execve(&fd->path, user_argv, user_envp);
        ufdt_put(ufd);
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

static task_node_t * find_child(task_node_t *me, pid_t pid) {
    task_node_t *o;
    LIST_FOR_EACH_ENTRY(o, &me->children, child_list) {
        if(o->pid == pid) {
            return o;
        }
    }

    return NULL;
}

//FIXME not remotely thread-safe
DEFINE_SYSCALL(getpid) {
    return obtain_task_node(current)->pid;
}

//FIXME not remotely thread-safe
DEFINE_SYSCALL(getppid) {
    task_node_t *parent = obtain_task_node(current)->parent;
    return parent ? parent->pid : 0;
}

#define WNOHANG 1
#define WUNTRACED 2

DEFINE_SYSCALL(waitpid, pid_t pid, int *stat_loc, int options) {
    task_node_t *node = obtain_task_node(current);

    pid_t cpid = -1;
    if(pid == -1) {
        task_node_t *zombie;
        zombie = reap_zombie(node);

        if(!zombie) {
            if(options & WNOHANG) {
                return 0;
            } else {
                if(!wait_for_condition(zombie = reap_zombie(node))) {
                    return -EINTR;
                }
            }
        }

        if(stat_loc) {
            *stat_loc = MK_STATCODE(zombie->exit_code, zombie->exit_cause);
        }

        cpid = zombie->pid;
    } else if (pid > 0) {
        task_node_t *child = find_child(node, pid);

        BUG_ON(!child);

        if(!wait_for_condition(atomic_read(&child->exit_state) == TASK_EXITED)) {
            return -EINTR;
        }

        cpid = pid;
    } else {
        UNIMPLEMENTED();
    }

    return cpid;
}

DEFINE_SYSCALL(getcwd, char *buff, size_t size) {
    path_t pwd = obtain_fs_context(current)->pwd;
    uint32_t path_len = 0;

    path_t cur = pwd;
    while(cur.dentry->parent || cur.mount->parent) {
        if(!cur.dentry->parent) {
              BUG_ON(cur.dentry != cur.mount->fs->root);
              cur.dentry = cur.mount->mountpoint;
              cur.mount = cur.mount->parent;
        }

        // +1 for DIRECTORY_SEPARATOR, not null byte
        path_len += strlen(cur.dentry->name) + 1;
        cur.dentry = cur.dentry->parent;
    }

    bool at_root = !path_len;
    if(at_root) {
        path_len = 1;
    }

    // + 1 for null byte
    if(size < path_len + 1) {
        return -EINVAL;
    }
    buff[path_len] = 0;

    uint32_t pos = path_len;
    cur = pwd;
    while(cur.dentry && (cur.dentry->parent || cur.mount->parent || at_root)) {
        if(!cur.dentry->parent && cur.mount->parent) {
              BUG_ON(cur.dentry != cur.mount->fs->root);
              cur.dentry = cur.mount->mountpoint;
              cur.mount = cur.mount->parent;
        }

        uint32_t seg_len = strlen(cur.dentry->name);
        pos -= seg_len + 1;
        buff[pos] = DIRECTORY_SEPARATOR;
        memcpy(buff + pos + 1, cur.dentry->name, seg_len);
        cur.dentry = cur.dentry->parent;
    }

    BUG_ON(pos != 0);

    return (uint32_t) buff;
}

static int32_t do_chdir(path_t *path) {
    if(!S_ISDIR(path->dentry->inode->mode)) {
        return -ENOTDIR;
    }

    //FIXME this desperately needs to be locked
    obtain_fs_context(current)->pwd = *path;
    return 0;
}

DEFINE_SYSCALL(chdir, const char *pathname) {
    if(!pathname) {
        return -EFAULT;
    }

    path_t path;
    int32_t ret = vfs_lookup(&obtain_fs_context(current)->pwd, pathname, &path);
    if(ret) {
        return ret;
    }

    return do_chdir(&path);
}

DEFINE_SYSCALL(fchdir, ufd_idx_t ufd) {
    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    return do_chdir(&fd->path);
}

static int32_t do_chown(path_t *path, uid_t owner, gid_t group) {
    if(owner != 0 || group != 0) {
        kprintf("syscall - users/groups not implemented!");
        return -ENOSYS;
    }

    //FIXME update appropriate inode times
    return 0;
}

DEFINE_SYSCALL(chown, const char *pathname, uid_t owner, gid_t group) {
    if(!pathname) {
        return -EFAULT;
    }

    path_t path;
    int32_t ret = vfs_lookup(&obtain_fs_context(current)->pwd, pathname, &path);
    if(ret) {
        return ret;
    }

    return do_chown(&path, owner, group);
}

DEFINE_SYSCALL(fchown, ufd_idx_t ufd, uid_t owner, gid_t group) {
    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    return do_chown(&fd->path, owner, group);
}

DEFINE_SYSCALL(seek, ufd_idx_t ufd, off_t off, int whence) {
    if(((uint32_t) whence) > SEEK_MAX) {
        return -EINVAL;
    }

    int32_t ret = -EBADF;

    file_t *fd = ufdt_get(ufd);
    if(fd) {
        //TODO sanitize buffer/size arguments

        ret = vfs_seek(fd, off, whence);

        ufdt_put(ufd);
    }

    return ret;
}

DEFINE_SYSCALL(_register_sigtramp, void (*tramp)(void *)) {
    void (**sigtramp)(void *) = &obtain_task_node(current)->sigtramp;
    if(*sigtramp) {
        return -1;
    }
    *sigtramp = tramp;
    return 0;
}

DEFINE_SYSCALL(_sigreturn, void *sp) {
    arch_setup_sigreturn(state, sp);
    return 0; //This isn't used
}

DEFINE_SYSCALL(kill, int pid, int sig) {
    panic("syscall - kill");
    return -ENOSYS;
}

DEFINE_SYSCALL(sigaction, int sig, const struct sigaction *restrict sa, struct sigaction *restrict sa_old) {
    if(sig < 0 || sig > NSIG) {
        return -EINVAL;
    }

    if(sa_old) {
        *sa_old = obtain_task_node(current)->sigactions[sig];
    }

    if(sa) {
        obtain_task_node(current)->sigactions[sig] = *sa;
    }

    return 0;
}

DEFINE_SYSCALL(sigprocmask, int how, const sigset_t *set, sigset_t *oset) {
    //FIXME take action based on current->node->sig_mask

    if(oset) {
        *oset = current->node->sig_mask;
    }

    switch(how) {
        case SIG_BLOCK: {
            if(set) current->node->sig_mask |= *set;
            break;
        }
        case SIG_UNBLOCK: {
            if(set) current->node->sig_mask &= *set;
            break;
        }
        case SIG_SETMASK: {
            if(set) current->node->sig_mask = *set;
            break;
        }
        default: return -EINVAL;
    }

    return 0;
}

DEFINE_SYSCALL(getpgid, pid_t pid) {
    if(pid == 0 || pid == current->node->pid) {
        return current->node->pgroup->leader->pid;
    }

    task_node_t *node = task_node_find(pid);
    if(!node) {
        return -ESRCH;
    }

    pid_t ret = node->pgroup->leader->pid;
    task_node_put(node);
    return ret;
}

DEFINE_SYSCALL(setpgid, pid_t pid, pid_t pgid) {
    if(pid < 0 || pgid < 0) {
        return -EINVAL;
    }

    task_node_t *node;

    //determine the node having its group changed
    if(pid != 0 && pid != current->node->pid) {
        node = task_node_find(pid);
        task_node_t *cur = node;

        //verify that node is us or our child
        while(cur) {
            if(cur->pid == current->node->pid) {
                break;
            }
            cur = cur->parent;
        }

        if(!cur) {
            return -EACCES;
        }

        //FIXME make sure that this child has not execve'd after fork before
        //this!
    } else {
        node = current->node;
        task_node_get(current->node);
    }

    if(pgid == 0 || pgid == node->pid) {
        pgroup_rm(node);
        pgroup_create(node);
    } else {
        pgroup_t *pg = pgroup_find(pgid);
        if(!pg) {
            return -EPERM;
        }

        //FIXME check that pg is in the same session
        pgroup_rm(node);
        pgroup_add(node, pg);
    }

    task_node_put(node);
    return 0;
}

DEFINE_SYSCALL(tcgetpgrp, ufd_idx_t ufd) {
    //FIXME CHECK THIS IS A TTY!

    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    return tty_get_pgroup(fd)->leader->pid;
}

DEFINE_SYSCALL(tcsetpgrp, ufd_idx_t ufd, pid_t pgid) {
    //FIXME CHECK THIS IS A TTY!

    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    pgroup_t *pg = pgroup_find(pgid);
    if(!pg) {
        return -EPERM;
    }

    tty_set_pgroup(fd, pg);

    return 0;
}

DEFINE_SYSCALL(dup, ufd_idx_t ufd) {
    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    return ufdt_add(fd);
}

DEFINE_SYSCALL(dup2, ufd_idx_t ufd, ufd_idx_t ufd2) {
    file_t *fd = ufdt_get(ufd);
    if(!fd) {
        return -EBADF;
    }

    if(ufd != ufd2) {
        ufdt_replace(ufd2, fd);
    }

    return ufd2;
}

DEFINE_SYSCALL(gettimeofday, struct timeval *tv) {
    //FIXME sanitize tv ptr
    uint64_t now = uptime();
    tv->tv_sec = now / MILLIS_PER_SEC;
    tv->tv_usec = (now % MILLIS_PER_SEC) * MICROS_PER_MILLI;
    return 0;
}

DEFINE_SYSCALL(unimplemented, char *msg, bool fatal) {
    if(fatal) {
        panicf("syscall - unimplemented: %s", msg);
    } else {
        kprintf("syscall - unimplemented: %s", msg);
    }
    return -ENOSYS;
}
