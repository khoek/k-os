#include <termios.h>
#include <string.h>
#include <k/sys.h>

uint8_t cc_code[] = {
  '\x3', '\x1C', '\x7F', '\x15', '\x4', '\x0', '\x1', '\x0', '\x11', '\x13',
  '\x1A', '\x0', '\x12', '\xF', '\x17', '\x16', '\x0', '\x0', '\x0', '\x0',
  '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0', '\x0',
  '\x0'
};

int tcgetattr(int fd, struct termios *t) {
    t->c_iflag = 042400;
    t->c_oflag = 05;
    t->c_cflag = 0277;
    t->c_lflag = 0105073;
    memcpy(t->c_cc, cc_code, NCCS);
    return 0;
}

int tcsetattr(int fd, int optact, const struct termios *t) {
    //FIXME we silently fail here (for bash)
    return 0;
}

int tcsendbreak(int fd, int duration) {
    return MAKE_SYSCALL(unimplemented, "tcsendbreak", false);
}

int tcdrain(int fd) {
    return MAKE_SYSCALL(unimplemented, "tcdrain", false);
}

int tcflush(int fd, int queue_selector) {
    return MAKE_SYSCALL(unimplemented, "tcflush", false);
}

int tcflow(int fd, int action) {
    return MAKE_SYSCALL(unimplemented, "tcflow", false);
}

void cfmakeraw(struct termios *t) {
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                | INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE | PARENB);
    t->c_cflag |= CS8;
}
