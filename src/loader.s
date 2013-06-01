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
.set KERNEL_PAGE_NUMBER, (KERNEL_VIRTUAL_BASE >> 22)

.section .data
.align 0x1000
page_directory:
    # This page directory entry identity-maps the first 4MB of the 32-bit physical address space.
    # All bits are clear except the following:
    # bit 7: PS The kernel page is 4MB.
    # bit 1: RW The kernel page is read/write.
    # bit 0: P  The kernel page is present.
    # This entry must be here -- otherwise the kernel will crash immediately after paging is
    # enabled because it can't fetch the next instruction! It's ok to unmap this page later.
    .long 0x00000083

    .rept KERNEL_PAGE_NUMBER - 1
    .long 0x00000083
    .endr

    .long 0x00000083

    .rept 1024 - KERNEL_PAGE_NUMBER - 1
    .long 0x00000083
    .endr

.align 32
.skip 0x400                             # reserve 16 KiB stack
stack:

.section .loader, "ax"
loader:
    # NOTE: Until paging is set up, the code must be position-independent and use physical
    # addresses, not virtual ones!
    movl $(page_directory - KERNEL_VIRTUAL_BASE), %ecx
    movl %ecx, %cr3                                     # Load Page Directory Base Register.

    movl %cr4, %ecx
    orl $0x00000010, %ecx                        # Set PSE bit in CR4 to enable 4MB pages.
    movl %ecx, %cr4

    movl %cr0, %ecx
    orl $0x80000000, %ecx                        # Set PG bit in CR0 to enable paging.
    movl %ecx, %cr0

    # Switch to the kernel mapping
    # approximately 0xC0100000.
    leal boot, %ecx
    jmp *%ecx                                   # NOTE: Must be absolute jump!
.size loader, .-loader

.text
.type boot, @function
boot:
    # Unmap the identity-mapped first 4MB of physical address space. It should not be needed
    # anymore.

    #movl $0, page_directory
    #invlpg 0

    movl  $stack, %esp                  # set up the stack, stacks grow downwards

    pushl %ebx                          # multiboot info
    pushl %eax                          # magic number

    call  kmain
.size boot, .-boot
