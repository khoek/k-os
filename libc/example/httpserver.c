#define NULL ((void *) 0)

#include <k/sys.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char *head_part1 = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: ";

char *head_part2 = "\r\n"
                   "Connection: close\r\n"
                   "\r\n";

char *page = "<!doctype html><html><head><title>K-OS</title></head><body><center><h1 style='font-size:90pt'>K-OS</h1><center><p>Welcome.</p></html>";

int main() {
    int fds = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(80);
    local.sin_addr.s_addr = INADDR_ANY;

    if(bind(fds, (struct sockaddr *) &local, sizeof(struct sockaddr_in))) {
        log("ERR: BIND");
        return 1;
    }

    if(listen(fds, 128)) {
        log("ERR: LIST");
        return 2;
    }

    while(1) {
        int fd = accept(fds, NULL, 0);

        char page_len[64];
        itoa(strlen(page), page_len, 10);

        send(fd, head_part1, strlen(head_part1), 0);
        send(fd, page_len, strlen(page_len), 0);
        send(fd, head_part2, strlen(head_part2), 0);
        send(fd, page, strlen(page), 0);

        shutdown(fd, SHUT_RDWR);

        close(fd);
    }

    return close(fds);
}
