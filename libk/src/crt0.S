.globl _start
_start:
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	mov $0, %ebp

	movl	$__bss_start, %edi
	movl	$_end, %ecx
	subl	%edi, %ecx
	xorl	%eax, %eax
	rep; stosb

	call _init_signal

/*
	* FIXME uncomment this when we implement signals for real (and in particular * implement the syscall "__install_signal_handler")
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

	pushl	%eax
	call	exit
3:
	jmp	3b
