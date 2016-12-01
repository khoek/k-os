#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

int isdigit(int c);

char * itoa(int value, char *buff, int base);
int atoi(char *str);

size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
char * strcpy (char * dest, const char * src);
char * strchr(char *str, const char c);

void * memset(void *ptr, int c, size_t bytes);
void * memcpy(void *dest, const void *source, size_t bytes);
void * memmove(void *dest, const void *source, size_t bytes);
int memcmp(const void *a, const void *b, size_t bytes);
void * memchr(void *ptr, int value, size_t bytes);

#endif
