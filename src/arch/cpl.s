.global cpl_switch

.type cpl_switch, @function
cpl_switch:
	mov 56(%esp), %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs

    add $4, %esp

    pop %eax
    mov %eax, %cr3

    popa
	iret
.size cpl_switch, .-cpl_switch
