.global cpl_switch

.type cpl_switch, @function
cpl_switch:
    add $4, %esp #Discard return address (the entire contents of this stack up to the last interrupt is about to be lost)

    pop %eax #new cr3
    pop %ecx #top of new stack (new esp)

    mov %eax, %cr3
    mov %ecx, %esp

	#FIXME change the segment registers depending on the value of ss

    #The next two instructions should load all registers off the new stack and then perform a task switch, regardless of whether we are going to kernel or user mode

    popa
	iret
.size cpl_switch, .-cpl_switch
