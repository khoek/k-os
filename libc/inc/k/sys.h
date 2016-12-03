#ifndef LIBC_K_SYS_H
#define LIBC_K_SYS_H

void _exit(int code);
void _msleep(unsigned int millis);
void _log(const char *s, unsigned short len);
unsigned long long _uptime();

int _alloc_page();
int _free_page();

#define PAGE_SIZE 4096

extern void *_data_start;
extern void *_data_end;

#endif
