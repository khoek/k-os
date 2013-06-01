.global loader                         # Make entry point visible to linker.

.extern kmain

# Multiboot header
.set ALIGN,    1 << 0                   # align loaded modules on page boundaries
.set MEMINFO,  1 << 1                   # provide memory map
.set FLAGS,    ALIGN | MEMINFO          # this is the multiboot 'flag' field
.set MAGIC,    0x1BADB002               # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS)         # checksum required

.section .header, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.set KERNEL_VIRTUAL_BASE, 0xC0000000

.section .init.data

.align 0x1000
page_table:
.set addr, 1
.rept 1024
.long addr
.set addr, addr+0x1000
.endr

.align 0x1000
page_directory:
.rept 1024
.long (page_table - KERNEL_VIRTUAL_BASE) + 1
.endr

.section .data

.align 32
.skip 0x400                             # reserve 16 KiB stack
stack:

.section .loader, "ax"
loader:
    movl $(page_table - KERNEL_VIRTUAL_BASE), %ecx
    orl $1, %ecx
    movl %ecx, (page_directory - KERNEL_VIRTUAL_BASE)

    movl $(page_directory - KERNEL_VIRTUAL_BASE), %ecx
    movl %ecx, %cr3

    movl %cr0, %ecx
    orl $0x80000000, %ecx                        # Set PG bit in CR0 to enable paging.
    movl %ecx, %cr0

    # Switch to the permanent kernel mapping
    leal boot, %ecx
    jmp *%ecx                                   # NOTE: Must be absolute jump!
.size loader, .-loader

.text
.type boot, @function
boot:
    movl  $stack, %esp                  # set up the stack, stacks grow downwards

    pushl %ebx                          # multiboot info
    pushl %eax                          # magic number

    call  kmain
.size boot, .-boot
