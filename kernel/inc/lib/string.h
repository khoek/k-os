#ifndef KERNEL_LIB_STRING_H
#define KERNEL_LIB_STRING_H

#include <stddef.h>
#include <stdarg.h>

#include "int.h"

int isdigit(int c);

char * itoa(int value, char *buff, int base);
int atoi(char *str);

size_t strlen(const char *str);
char * strchr(const char *str, int c);
int strcmp(const char *a, const char *b);
char * strcpy(char * dest, const char * src);
char * strdup(const char *orig);

void * memset(void *ptr, int c, size_t bytes);
void * memcpy(void *dest, const void *source, size_t bytes);
void * memmove(void *dest, const void *source, size_t bytes);
int memcmp(const void *a, const void *b, size_t bytes);
void * memchr(void *ptr, int value, size_t bytes);

#endif
