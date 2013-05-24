.global cpl_switch

.type cpl_switch, @function
cpl_switch:
	mov 4(%esp), %eax
	mov 52(%eax), %ds
	mov 52(%eax), %es
	mov 52(%eax), %fs
	mov 52(%eax), %gs

	push 52(%eax)
	push 48(%eax)
	push 44(%eax)
	push 40(%eax)
	push 36(%eax)

	iret
.size cpl_switch, .-cpl_switch
