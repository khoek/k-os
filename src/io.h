#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
	volatile uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) );
	return ret;
}

static inline void outl(uint16_t port, uint32_t value) {
	__asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
	volatile uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port) );
	return ret;
}

static inline void insw(uint16_t port, void * buff, uint32_t size) {
    __asm__ volatile("rep insw" : "+D"(buff), "+c"(size) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, void * buff, uint32_t size) {
    __asm__ volatile("rep outsw" : "+S"(buff), "+c"(size) : "d"(port));
}

static inline void insl(uint16_t port, void * buff, uint32_t size) {
    __asm__ volatile("rep insl" : "=D" (buff), "=c" (size) : "d" (port), "0" (buff), "1" (size));
}

static inline void outsl(uint16_t port, void * buff, uint32_t size) {
    __asm__ volatile("rep outsl" : "=S" (buff), "=c" (size) : "d" (port), "0" (buff), "1" (size));
}
