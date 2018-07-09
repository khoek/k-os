#include <netdb.h>
#include <k/sys.h>

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return MAKE_SYSCALL(unimplemented, "getaddrinfo", true);
}

void freeaddrinfo(struct addrinfo *res) {
    MAKE_SYSCALL(unimplemented, "freeaddrinfo", true);
}

const char * gai_strerror(int errcode) {
    return (const char *) MAKE_SYSCALL(unimplemented, "gai_strerror", true);
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags) {
    return MAKE_SYSCALL(unimplemented, "getnameinfo", true);
}
