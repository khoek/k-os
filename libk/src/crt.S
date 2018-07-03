.globl __start
__start:
/*
	movl	$__stack, %eax
	testl	%eax, %eax
	jz	1f
	movl	%eax, %esp
*/

	pushl	%ebx
	pushl	%eax
	mov $0, %ebp

	movl	$__bss_start, %edi
	movl	$__bss_end, %ecx
	subl	%edi, %ecx
	xorl	%eax, %eax
	rep; stosb

	call _init_signal

/*
	pushl	$__sigtramp
	pushl	$0
	call	__install_signal_handler
	popl	%eax
*/
	pushl	$__do_global_dtors
	call	atexit
	popl	%eax
	call	__do_global_ctors

	call	main
	popl	%ecx
	popl	%edx

	/* call exit from the C library so atexit gets called, and the
	 * C++ destructors get run. This calls our exit routine below
	 * when it's done.
	 */
	pushl	%eax
	call	exit
3:
	jmp	3b
