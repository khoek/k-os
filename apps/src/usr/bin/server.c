#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LINE_ENDING "\r\n"

static int send_buff_front = 0;
static char send_buff[4096];

static void append_str(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    send_buff_front += vsprintf(send_buff + send_buff_front, fmt, va);
    va_end(va);
}

static void append_binary(char *data, int len) {
    memcpy(send_buff + send_buff_front, data, len);
    send_buff_front += len;
}

static void commit_buffer(int fd) {
    send(fd, send_buff, send_buff_front, 0);
}

static bool recv_line(int fd, char *buff) {
    char c;
    if(recv(fd, &c, 1, 0) == -1) {
        return false;
    }
    while(c != '\r') {
        *buff++ = c;
        if(recv(fd, &c, 1, 0) == -1) {
            return false;
        }
    }
    *buff = '\0';
    if(recv(fd, &c, 1, 0) == -1) {
        return false;
    }
    return true;
}

static int read_file(int fid, char *buff) {
    int len = 0;
    while(read(fid, &buff[len], 1)) {
        len++;
    }
    return len;
}

static const char * get_type(char *res) {
    char *rid = strrchr(res, '.');
    if(rid++) {
        if(!strcmp(rid, "html")) {
            return "text/html";
        } else if(!strcmp(rid, "ico")) {
            return "image/vnd.microsoft.icon";
        }
    }

    return "application/octet-stream";
}

char line[512];
char content[4096];
static void handle_conn(int fd) {
    if(!recv_line(fd, line)) {
        return;
    }

    char *method = line;
    char *res = strchr(method, ' ');
    *res++ = '\0';
    char *protocol = strchr(res, ' ');
    *protocol++ = '\0';

    printf("(%s) %s:%s\n", protocol, method, res);

    if(!strcmp(method, "GET") && *res) {
        if(!strcmp(res, "/")) {
            res = "/index.html";
        }

        res++;

        char path[100];
        sprintf(path, "/var/www/%s", res);
        int fid = open(path, 0);

        if(fid != -1) {
            int len = read_file(fid, content);
            close(fid);

            append_str("%s 200 OK" LINE_ENDING, protocol);
            append_str("Content-Type: %s" LINE_ENDING, get_type(res));
            append_str("Content-Length: %u" LINE_ENDING, len);
            append_str("Connection: close" LINE_ENDING LINE_ENDING);

            append_binary(content, len);
        } else {
            append_str("%s 404 Not Found" LINE_ENDING, protocol);
            append_str("Connection: close" LINE_ENDING LINE_ENDING);
        }
    } else {
        append_str("%s 501 Not Implemented" LINE_ENDING, protocol);
    }

    commit_buffer(fd);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

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

    printf("HTTP: listen() succeeded\n");

    while(1) {
        int fd = accept(fds, NULL, 0);

        //nonblocking collect zombies

        if(!fork()) {
            handle_conn(fd);
            return 0;
        }
    }

    return close(fds);
}
