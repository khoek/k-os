.global _exit
.global _fork
.global _sleep
.global _log
.global _uptime

.global open
.global close
.global socket
.global listen
.global accept
.global bind
.global connect
.global shutdown
.global send
.global recv

.global _alloc_page
.global _free_page

.global stat
.global lstat
.global fstat

.global play
.global stop

.global read
.global write

_exit:
    mov $0, %eax
    mov 4(%esp), %ecx
    int $0x80

    ret

_fork:
    mov $1, %eax
    mov 4(%esp), %ecx
    int $0x80

    ret

_sleep:
    mov $2, %eax
    mov 4(%esp), %ecx
    int $0x80

    ret

_log:
    mov $3, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

_uptime:
    mov $4, %eax
    int $0x80

    ret

open:
    mov $5, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

close:
    mov $6, %eax
    mov 4(%esp), %ecx
    int $0x80

    ret

socket:
    push %ebx

    mov $7, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret

listen:
    mov $8, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

accept:
    push %ebx

    mov $9, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret

bind:
    push %ebx

    mov $10, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret

connect:
    push %ebx

    mov $11, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret

shutdown:
    mov $12, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

send:
    push %esi
    push %ebx

    mov $13, %eax
    mov 24(%esp), %ebx
    mov 20(%esp), %ebx
    mov 16(%esp), %edx
    mov 12(%esp), %ecx
    int $0x80

    pop %ebx
    pop %esi
    ret

recv:
    push %esi
    push %ebx

    mov $14, %eax
    mov 24(%esp), %ebx
    mov 20(%esp), %ebx
    mov 16(%esp), %edx
    mov 12(%esp), %ecx
    int $0x80

    pop %ebx
    pop %esi
    ret

_alloc_page:
    mov $15, %eax
    int $0x80

    ret

_free_page:
    mov $16, %eax
    int $0x80

    ret

stat:
    mov $17, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

lstat:
    mov $18, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

fstat:
    mov $19, %eax
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    int $0x80

    ret

play:
    mov $20, %eax
    mov 4(%esp), %ecx
    int $0x80

    ret

stop:
    mov $21, %eax
    int $0x80

    ret

read:
    push %ebx

    mov $22, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret

write:
    push %ebx

    mov $23, %eax
    mov 16(%esp), %ebx
    mov 12(%esp), %edx
    mov 8(%esp), %ecx
    int $0x80

    pop %ebx
    ret
