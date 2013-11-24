.global cpl_switch

.type cpl_switch, @function
cpl_switch:
    # Discard return address
    add $4, %esp

    # Load parameters
    pop %ecx # new cr3
    pop %eax # new eax (for returning values from syscalls)
    pop %edx # new edx (for returning 64-bit values from syscalls)
    pop %esp # new stack

    # Switch page tables
    mov %ecx, %cr3

    # Load the new CS into ecx and test whether we are going to usermode
    # (by masking the bottom 3 bits). If we aren't, skip reloading the
    # segment registers.
    mov 36(%esp), %ecx
    and $0x7, %ecx
    jz .finish

    # Reload the segment registers (all but CS and SS) with the new SS.
    # CS and SS will be reloaded when we iret.
    mov 48(%esp), %ecx
    mov %ecx, %ds
    mov %ecx, %es
    mov %ecx, %fs
    add $0x8, %ecx
    mov %ecx, %gs

.finish:
    # Preserve eax and edx after popa
    mov %eax, 28(%esp)
    mov %edx, 20(%esp)

    # The next two instructions should load all registers off the new stack
    # and then perform a task switch, regardless of whether we are going to
    # kernel or user mode
    popa
	iret
.size cpl_switch, .-cpl_switch
