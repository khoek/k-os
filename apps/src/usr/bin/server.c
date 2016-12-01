#define NULL ((void *) 0)

#include <k/sys.h>
#include <k/log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LINE_ENDING "\r\n"

char send_str_buff[2048];
static void send_str(int fd, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vsprintf(send_str_buff, fmt, va);
    va_end(va);

    send(fd, send_str_buff, strlen(send_str_buff), 0);
}

char content[2048];
int main() {
    int fds = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(80);
    local.sin_addr.s_addr = INADDR_ANY;

    if(bind(fds, (struct sockaddr *) &local, sizeof(struct sockaddr_in))) {
        printf("ERR: bind");
        return 1;
    }

    if(listen(fds, 128)) {
        printf("ERR: listen");
        return 2;
    }

    printf("HTTP: listen() succeeded");

    int client_num = 1;
    while(1) {
        int fd = accept(fds, NULL, 0);
        int len = sprintf(content, "<!doctype html>"
                            "<html><head><title>K-OS</title></head><body><center><h1 style='font-size:90pt'>K-OS</h1><p>Welcome. You are client number %u.</p></center></html>", client_num);

        printf("HTTP: accept(), client #%u\n", client_num);

        send_str(fd, "HTTP/1.1 200 OK" LINE_ENDING);
        send_str(fd, "Content-Type: text/html" LINE_ENDING);
        send_str(fd, "Content-Length: %u" LINE_ENDING, len);
        send_str(fd, "Connection: close" LINE_ENDING LINE_ENDING);

        send_str(fd, content);

        shutdown(fd, SHUT_RDWR);

        close(fd);

        printf("HTTP: close()\n");

        client_num++;
    }

    return close(fds);
}
