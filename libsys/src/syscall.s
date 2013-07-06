.global _exit
.global _fork
.global _sleep
.global _log
.global _uptime

_exit:
    mov 4(%esp), %ecx
    mov $0, %eax
    int $0x80

    ret

_fork:
    mov 4(%esp), %ecx
    mov $1, %eax
    int $0x80

    ret

_sleep:
    mov 4(%esp), %ecx
    mov $2, %eax
    int $0x80

    ret

_log:
    mov 8(%esp), %edx
    mov 4(%esp), %ecx
    mov $3, %eax
    int $0x80

    ret

_uptime:
    mov $4, %eax
    int $0x80

    ret
