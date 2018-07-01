.global perform_syscall_0
.global perform_syscall_1
.global perform_syscall_2
.global perform_syscall_3
.global perform_syscall_4
.global perform_syscall_5

perform_syscall_0:
    int $0x80

    ret

perform_syscall_1:
    mov 4(%esp), %ecx
    int $0x80

    ret

perform_syscall_2:
    mov 4(%esp), %ecx
    mov 8(%esp), %edx
    int $0x80

    ret

perform_syscall_3:
    push %ebx

    mov 8(%esp), %ecx
    mov 12(%esp), %edx
    mov 16(%esp), %ebx
    int $0x80

    pop %ebx
    ret

perform_syscall_4:
    push %ebx
    push %esi

    mov 12(%esp), %ecx
    mov 16(%esp), %edx
    mov 20(%esp), %ebx
    mov 24(%esp), %esi
    int $0x80

    pop %esi
    pop %ebx
    ret

perform_syscall_5:
    push %ebx
    push %esi
    push %edi

    mov 16(%esp), %ecx
    mov 20(%esp), %edx
    mov 24(%esp), %ebx
    mov 28(%esp), %esi
    mov 32(%esp), %edi
    int $0x80

    pop %edi
    pop %esi
    pop %ebx
    ret
