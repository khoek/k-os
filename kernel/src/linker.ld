OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(entry)
SECTIONS {
    . = 0x00100000;
    image_start = .;
    .header : {
       *(.header)
    }
    .entry : {
       *(.entry)
    }
    .entry.ap ALIGN (0x1000) : {
        entry_ap_start = .;
        *(.entry.ap)
        entry_ap_end = .;
    }
    entry_ap_flush = unmapped_entry_ap_flush - entry_ap_start + 0x8000;
    boot_gdt = unmapped_boot_gdt - entry_ap_start + 0x8000;
    boot_gdt_end = unmapped_boot_gdt_end - entry_ap_start + 0x8000;
    boot_gdtr = unmapped_boot_gdtr - entry_ap_start + 0x8000;
    . += 0xC0000000;
    .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
        *(.rodata*)
    }
    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)
    }
    .data.percpu ALIGN(0x1000) : AT(ADDR(.data.percpu) - 0xC0000000) {
        percpu_data_start = .;
        *(.data.percpu)
        percpu_data_end = .;
    }
    .bss : AT(ADDR(.bss) - 0xC0000000) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
    }
    .strtab : AT(ADDR(.strtab) - 0xC0000000) {
        *(.strtab)
    }
    .symtab : AT(ADDR(.symtab) - 0xC0000000) {
        *(.symtab)
    }
    .init.text ALIGN (0x1000) : AT(ADDR(.init.text) - 0xC0000000) {
        init_mem_start = . - 0xC0000000;
        *(.init.text)
    }
    .init.data ALIGN (0x1000) : AT(ADDR(.init.data) - 0xC0000000) {
        *(.init.data)
    }
    .init.call ALIGN (0x1000) : AT(ADDR(.init.call) - 0xC0000000) {
        initcall_start = .;
        *(.init.call.0)
        *(.init.call.1)
        *(.init.call.2)
        *(.init.call.3)
        *(.init.call.4)
        *(.init.call.5)
        *(.init.call.6)
        *(.init.call.7)
        *(.init.call.8)
        *(.init.call.9)
        *(.init.call.10)
        initcall_end = .;
    }
    .init.param ALIGN (0x1000) : AT(ADDR(.init.param) - 0xC0000000) {
        param_start = .;
        *(.init.param)
        param_end = .;
    }
    init_mem_end = . - 0xC0000000;
    image_end = . - 0xC0000000;
    /DISCARD/ : {
        *(.comment)
        *(.note.gnu.build-id)
    }
}
