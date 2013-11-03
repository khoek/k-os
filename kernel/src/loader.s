.global entry                           # UP/BSP entry point
.global entry_ap                        # AP entry point (page aligned)

.extern kmain                           # UP/BSP startup
.extern mp_ap_init                      # AP entry startup

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

.section .data

.align 32
.skip 0x400  # 16 KiB
boot_stack:

.align 32
.skip 0x400  # 16 KiB
boot_stack_ap:

.section .init.data

.align 0x1000
boot_page_table:
.set addr, 1
.rept 1024
.long addr
.set addr, addr+0x1000
.endr

.align 0x1000
boot_page_directory:
.rept 1024
.long (boot_page_table - KERNEL_VIRTUAL_BASE) + 1
.endr

.section .entry, "ax"
.align 16
boot_gdt: 
.word 0x0000,0x0000,0x0000,0x0000        # Null desciptor
.word 0xFFFF,0x0000,0x9A00,0x00CF        # 32-bit code descriptor
.word 0xFFFF,0x0000,0x9200,0x00CF        # 32-bit data descriptor
boot_gdt_end:

.align 16
boot_gdtr:
.word boot_gdt_end - boot_gdt - 1               
.long boot_gdt

entry:
    # Disable interrupts
    cli
    
    # Install temporary higher-half page directory
    movl $(boot_page_directory - KERNEL_VIRTUAL_BASE), %ecx
    movl %ecx, %cr3

    # Enable paging
    movl %cr0, %ecx
    orl $0x80000000, %ecx
    movl %ecx, %cr0

    # Enter higher-half
    leal boot_bsp, %ecx
    jmp *%ecx             # NOTE: Must be absolute jump!
.size entry, .-entry

.section .entry.ap, "ax"
entry_ap:
.code16
    # Disable interrupts
    cli
    
    # Enable the A20 line using the BIOS
    mov $0x2401, %ax 
    int $0x15
    
    lgdt boot_gdtr
    
    # Enter Protected Mode
    mov %cr0, %ecx
    or $0x00000001, %cx
    mov %ecx, %cr0
    
    # Flush code segment selector
    jmp $0x08, $.flush
.flush:
.code32
    # Flush data segment selectors
    mov $0x10, %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Install temporary higher-half page directory
    movl $(boot_page_directory - KERNEL_VIRTUAL_BASE), %ecx
    movl %ecx, %cr3

    # Enable paging
    mov %cr0, %ecx
    or $0x80000000, %ecx  
    mov %ecx, %cr0
    
    # Enter higher-half
    lea boot_ap, %ecx
    jmp *%ecx             # NOTE: Must be absolute jump!
.size entry_ap, .-entry_ap

.text
.type boot, @function
boot_bsp:
    # Set up the BSP stack
    movl $boot_stack, %esp

    # Multiboot arguments
    pushl %ebx  # multiboot info
    pushl %eax  # magic number

    # C entry point
    call kmain
.size boot_bsp, .-boot_bsp

.type boot_ap, @function
boot_ap:    
    # Set up the AP stack
    mov $boot_stack_ap, %esp  
    
    # C entry point
    call mp_ap_init
.size boot_ap, .-boot_ap
