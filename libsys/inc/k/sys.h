#ifndef LIBSYS_K_SYS_H
#define LIBSYS_K_SYS_H

void _exit(int code);
unsigned long long _fork(unsigned int param);
void _sleep(unsigned int millis);
void _log(const char *s, unsigned short len);
unsigned long long _uptime();

#endif
