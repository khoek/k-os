#include <k/syscall.h>

//FIXME ensure 16-byte stack alignment!

.globl _start
_start:
	mov %ecx, environ
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	mov $0, %ebp

	movl $__bss_start, %edi
	movl $_end, %ecx
	subl %edi, %ecx
	xorl %eax, %eax
	rep; stosb

	pushl	$_sigtramp
	call SYSCALL(_register_sigtramp)
	popl %eax

/* FIXME How do we register the correct exit handler which gcc wants? */
	pushl $__do_global_dtors
	call atexit
	popl %eax
	call __do_global_ctors

	call main
	popl %ecx
	popl %edx

	pushl	%eax
	call  exit
3:
	jmp	3b
