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
char cc[64];
itoa(str, cc, 16);
        kprint(cc);
        itoa(strlen(str), cc, 16);
                kprint(cc);
    send(fd, str, strlen(str), 0);
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

        kprint("INFO: accept() succeeded");
char cc[64];
itoa(fd, cc, 10);
        kprint(cc);

                        kprint("INFO: 1");
        send_str(fd, "HTTP/1.1 200 OK" LINE_ENDING);
                        kprint("INFO: 1a");
        send_str(fd, "Content-Type: text/html" LINE_ENDING);
                        kprint("INFO: 1b");
        send_str(fd, "Content-Length: ");
                        kprint("INFO: 1c");

                kprint("INFO: 2");
        char content_len[64];
        itoa(strlen(content), content_len, 10);
                        kprint("INFO: 3");
        send_str(fd, content_len);
                        kprint("INFO: 4");
        send_str(fd, LINE_ENDING);

        send_str(fd, LINE_ENDING "Connection: close" LINE_ENDING LINE_ENDING);

        send_str(fd, content);

                        kprint("INFO: 5");
        shutdown(fd, SHUT_RDWR);

        kprint("INFO: shutdown()");

        close(fd);
    }

    return close(fds);
}
