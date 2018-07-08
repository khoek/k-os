#include <sys/socket.h>
#include <k/sys.h>

int accept(int fd, struct sockaddr *addr, socklen_t *len) {
  return MAKE_SYSCALL(accept, fd, addr, len);
}

int bind(int fd, const struct sockaddr *addr, socklen_t len) {
  return MAKE_SYSCALL(bind, fd, addr, len);
}

int connect(int fd, const struct sockaddr *addr, socklen_t len) {
  return MAKE_SYSCALL(connect, fd, addr, len);
}

int listen(int fd, int backlog) {
  return MAKE_SYSCALL(listen, fd, backlog);
}

ssize_t recv(int fd, void *buff, size_t len, int flags) {
  return MAKE_SYSCALL(recv, fd, buff, len, flags);
}

ssize_t send(int fd, const void *buff, size_t len, int flags) {
  return MAKE_SYSCALL(send, fd, buff, len, flags);
}

int shutdown(int fd, int how) {
  return MAKE_SYSCALL(shutdown, fd, how);
}

int socket(int family, int type, int protocol) {
  return MAKE_SYSCALL(socket, family, type, protocol);
}
