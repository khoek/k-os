#ifndef LIBSYS_H
#define LIBSYS_H

void sys_exit(int code);
unsigned long long sys_fork(unsigned int param);
void sys_sleep(unsigned int millis);
void sys_log(const char *s, unsigned short len);
unsigned long long sys_uptime();

#endif
