#include "stddef.h"
#include "string.h"

int isdigit(char c) {
    return (c >= '0') && (c <= '9');
}

char* itoa(int value, char* buff, int base) {
    char* charset = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* ret = buff;
    char scratch[64];
    int idx = 0;
    if(value < 0) {
        *buff++ = '-';
        value = -value;
    }
    if(value == 0) {
        *buff++ = '0';
        *buff = 0;
        return ret;
    }
    while(value > 0) {
        scratch[idx++] = charset[value % base];
        value /= base;
    }
    while(idx > 0) {
        *buff++ = scratch[--idx];
    }
    *buff = 0;
    return ret;
}

int atoi(char* str) {
    int i = 0;
    int factor = 1;
    if(*str == '-') {
        factor = -1;
        str++;
    }
    if(*str == '+') str++;
    while(*str) {
        if(*str < '0' || *str > '9') break;
        i *= 10;
        i += *str - '0';
        str++;
    }
    return i * factor;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while(*str++) len++;
    return len;
}

const char* strchr(const char* str, const char c) {
    while(*str++) {
        if(*str == c) {
            return str;
        }
    }

    return NULL;
}

int strcmp(const char* a, const char* b) {
    while(1) {
        if(*a == 0 && *b == 0) {
            return 0;
        }
        if(*a != *b) {
            return *a - *b;
        }
        a++;
        b++;
    }
}

void* memset(void* ptr, char c, size_t bytes) {
    char* p = (char*)ptr;
    size_t i;
    for(i = 0; i < bytes; i++) {
        p[i] = c;
    }
    return ptr;
}

void* memcpy(void* dest, const void* source, size_t bytes) {
    char* d = (char*)dest;
    const char* s = (const void*)source;
    size_t i;
    for(i = 0; i < bytes; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* source, size_t bytes) {
    char* d = (char*)dest;
    const char* s = (const void*)source;
    size_t i;
    if(d < s) {
        for(i = 0; i < bytes; i++) {
            d[i] = s[i];
        }
    } else {
        for(i = bytes; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void* a, const void* b, size_t bytes) {
    const char* ca = (const char*)a;
    const char* cb = (const char*)b;
    size_t i;
    for(i = 0; i < bytes; i++) {
        if(ca[i] != cb[i]) {
            return ca[i] - cb[i];
        }
    }
    return 0;
}

void* memchr(void* ptr, int value, size_t bytes) {
    size_t i;
    uint8_t* p = (uint8_t*)ptr;
    uint8_t c = (uint8_t)value;
    for(i = 0; i < bytes; i++, p++) {
        if(*p == c) {
            return p;
        }
    }
    return NULL;
}
