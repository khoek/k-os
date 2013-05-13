.global reload_segment_registers

.type   reload_segment_registers, @function
reload_segment_registers: 
    jmp $0x08, $flush_cs

.type   flush_cs, @function
flush_cs: 
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret

