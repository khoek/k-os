.global flush_segment_registers
.global flush_tss
.global enter_usermode

.extern gdt_set_kernel_stack
.extern task_usermode

.type flush_segment_registers, @function
flush_segment_registers:
    jmp  $0x08, $flush_data_segments
.size flush_segment_registers, .-flush_segment_registers

.type flush_cs, @function
flush_data_segments:
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret
.size flush_data_segments, .-flush_data_segments

.type flush_tss, @function
flush_tss:
   mov $0x2B, %ax
   ltr %ax
   ret
.size flush_tss, .-flush_tss

.type enter_usermode, @function
enter_usermode:
	mov $0x23, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs

	mov %esp, %eax
	push $0x23
	push %eax
	pushf
	push $0x1B
	push $task_usermode

	iret
.size enter_usermode, .-enter_usermode