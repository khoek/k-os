.global cpl_switch

.type cpl_switch, @function
cpl_switch:
    # Discard return address
    add $4, %esp

    # Load parameters
    pop %eax #new cr3
    pop %esp

    # Switch page tables
    mov %eax, %cr3

    # Load the new CS into eax and test whether we are going to usermode
    # (by masking the bottom 3 bits). If we are, load the segment registers
    # (all but CS, SS) with the SS
    mov 36(%esp), %eax

    and $0x7, %eax

    jz finish

    mov 48(%esp), %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs

finish:
    # The next two instructions should load all registers off the new stack and then perform a task
    # switch, regardless of whether we are going to kernel or user mode

    popa
	iret
.size cpl_switch, .-cpl_switch
