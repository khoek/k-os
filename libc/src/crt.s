.global _start

.extern main
.extern _exit

.extern _data_start
.extern _data_end

.section .text.entry, "ax"

_start:
#    pop %eax
#    mov %eax, _data_start
#    mov %eax, _data_end

    call main
    push %eax
    call _exit
