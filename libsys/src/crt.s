.global _start

.extern main
.extern _exit

_start:
    call main
    push %eax
    call _exit
