.global entry                           # UP/BSP entry point
.global entry_ap                        # AP entry point (page aligned)

.global unmapped_entry_ap_flush
.global unmapped_boot_gdt
.global unmapped_boot_gdt_end
.global unmapped_boot_gdtr

.extern kmain                           # UP/BSP startup
.extern mp_ap_start                     # AP entry startup

.extern init_page_directory                  # AP startup page directory
.extern next_ap_stack                   # AP startup stack

.extern entry_ap
.extern entry_ap_flush
.extern boot_gdt
.extern boot_gdt_end
.extern boot_gdtr

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

.section .data

.align 32
.skip 0x4000    # 16 kB
boot_stack:

.section .init.data

.align 0x1000
boot_page_table:
.set addr, 0
.rept 1024
.long addr | 1
.set addr, addr + 0x1000
.endr

.align 0x1000
boot_page_directory:
.rept 1024
.long (boot_page_table - 0xC0000000) + 1
.endr

.section .entry, "ax"
entry:
    # Disable interrupts
    cli

    # Install temporary higher-half page directory
    mov $(boot_page_directory - 0xC0000000), %ecx
    mov %ecx, %cr3

    # Enable paging
    mov %cr0, %ecx
    or $0x80000000, %ecx
    mov %ecx, %cr0

    # Enter higher-half
    lea boot_bsp, %ecx
    jmp *%ecx             # NOTE: Must be absolute jump!
.size entry, .-entry

.section .entry.ap, "ax"
entry_ap:
.code16
    # Disable interrupts
    cli

    # Enable the A20 line (FAST A20)
    in $0x92, %al
    or $2, %al
    out %al, $0x92

    # Load the GDT
    xorl  %eax, %eax
    movw  %ds, %ax
    shll  $4, %eax
    addl  $boot_gdt, %eax
    movl  %eax, boot_gdtr + 2
    movl  $boot_gdt_end, %eax
    subl  $boot_gdt, %eax
    movw  %ax, boot_gdtr
    lgdt boot_gdtr

    # Enter Protected Mode
    mov %cr0, %ecx
    or $1, %ecx
    mov %ecx, %cr0

    # Flush code segment selector
    jmp $0x8, $entry_ap_flush
unmapped_entry_ap_flush:
.code32
    # Flush data segment selectors
    mov $0x10, %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Install temporary higher-half page directory
    mov $(boot_page_directory - 0xC0000000), %ecx
    mov %ecx, %cr3

    # Enable paging
    mov %cr0, %ecx
    or $0x80000000, %ecx
    mov %ecx, %cr0

    # Enter higher-half
    lea boot_ap, %ecx
    jmp *%ecx             # NOTE: Must be absolute jump!
.size entry_ap, .-entry_ap

.align 16
unmapped_boot_gdt:
    .word 0x0000,0x0000,0x0000,0x0000        # Null desciptor
    .word 0xFFFF,0x0000,0x9A00,0x00CF        # 32-bit code descriptor
    .word 0xFFFF,0x0000,0x9200,0x00CF        # 32-bit data descriptor
unmapped_boot_gdt_end:

    .align 16
unmapped_boot_gdtr:
    .word 0
    .long 0

.text
.type boot_bsp, @function
boot_bsp:
    # Set up the BSP stack
    mov $boot_stack, %esp

    # Multiboot arguments
    push %ebx  # multiboot info
    push %eax  # magic number

    # C entry point
    call kmain
.size boot_bsp, .-boot_bsp

.type boot_ap, @function
boot_ap:
    # Set up the AP page directory
    mov $(init_page_directory - 0xC0000000), %ecx
    mov %ecx, %cr3

    # Set up the AP stack
    mov next_ap_stack, %esp

    # C entry point
    call mp_ap_start
.size boot_ap, .-boot_ap
