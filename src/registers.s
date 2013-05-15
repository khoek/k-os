.global reload_segment_registers
.global set_data_segments

.type reload_segment_registers, @function
reload_segment_registers:
    jmp  $0x08, $reload_data_segments
.size reload_segment_registers, .-reload_segment_registers

.type flush_cs, @function
reload_data_segments:
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret
.size reload_data_segments, .-reload_data_segments

