global loader                           ; making entry point visible to linker
global magic                            ; we will use this in kmain
global mbd                              ; we will use this in kmain
 
extern kmain                            ; kmain is defined in kmain.cpp
 
; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
FLAGS       equ  MODULEALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002             ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required
 
section .text
 
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
 
; reserve initial kernel stack space
STACKSIZE equ 0x4000                    ; that's 16k.
 
loader:
    mov  esp, stack + STACKSIZE         ; set up the stack
    push eax                            ; Multiboot magic number
    push ebx                            ; Multiboot info structure
 
    call kmain                          ; call kernel proper

    cli                                 ; clear interrupt flag and hang
.hang:
    hlt                                 ; halt forever
    jmp  .hang
 
section .bss 

align 4
stack: resb STACKSIZE                   ; reserve 16k stack on a doubleword boundary
magic: resd 1
mbd:   resd 1
