.global loader

.set ALIGN,    1 << 0                   # align loaded modules on page boundaries
.set MEMINFO,  1 << 1                   # provide memory map
.set FLAGS,    ALIGN | MEMINFO          # this is the multiboot 'flag' field
.set MAGIC,    0x1BADB002               # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS)         # checksum required

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.skip 0x400                             # reserve 16 KiB stack
stack:

.type loader, @function
loader:
    movl  $stack, %esp                  # set up the stack, stacks grow downwards
    pushl %ebx                          # multiboot info
    pushl %eax                          # magic number

    call  kmain

    cli
.size loader, .-loader

.type hang, @function
hang:                                   # hang if kmain returns
    hlt
    jmp   hang
.size hang, .-hang
