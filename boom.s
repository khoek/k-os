.global _start

_start:
mov $1, %eax
mov $8, %ebx
int $0x80

mov $0, %eax
mov $-1, %ebx
int $0x80
