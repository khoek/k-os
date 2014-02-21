#define NULL ((void *) 0)

#include <k/sys.h>
#include <k/log.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const char *content = "<!doctype html>"
                    "<html><head><title>K-OS</title></head><body><center><h1 style='font-size:90pt'>K-OS</h1><center><p>Welcome.</p></html>";

#define LINE_ENDING "\r\n"

static void send_str(int fd, const char *str) {
    send(socket, str, strlen(str), 0);
}

int main() {
    int fds = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(80);
    local.sin_addr.s_addr = INADDR_ANY;

    if(bind(fds, (struct sockaddr *) &local, sizeof(struct sockaddr_in))) {
        kprint("ERR: bind");
        return 1;
    }

    if(listen(fds, 128)) {
        kprint("ERR: listen");
        return 2;
    }

    while(1) {
        int fd = accept(fds, NULL, 0);

        send_str(fd, "HTTP/1.1 200 OK" LINE_ENDING);
        send_str(fd, "Content-Type: text/html" LINE_ENDING);
        send_str(fd, "Content-Length: ");

        char content_len[64];
        itoa(strlen(content), content_len, 10);
        send_str(fd, content_len);
        send_str(fd, LINE_ENDING);

        send_str(fd, LINE_ENDING "Connection: close" LINE_ENDING LINE_ENDING);

        send_str(fd, content);

        shutdown(fd, SHUT_RDWR);

        close(fd);
    }

    return close(fds);
}
