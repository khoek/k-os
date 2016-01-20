#include "init/initcall.h"

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

ENTRY(entry)
SECTIONS {
    . = PHYSICAL_BASE;

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

    entry_ap_flush = unmapped_entry_ap_flush - entry_ap_start + ENTRY_AP_BASE;
    boot_gdt = unmapped_boot_gdt - entry_ap_start + ENTRY_AP_BASE;
    boot_gdt_end = unmapped_boot_gdt_end - entry_ap_start + ENTRY_AP_BASE;
    boot_gdtr = unmapped_boot_gdtr - entry_ap_start + ENTRY_AP_BASE;

    . += VIRTUAL_BASE;

    .text ALIGN (0x1000) : AT(ADDR(.text) - VIRTUAL_BASE) {
        *(.text)
        *(.rodata*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - VIRTUAL_BASE) {
        *(.data)
    }

    .data.percpu ALIGN(0x1000) : AT(ADDR(.data.percpu) - VIRTUAL_BASE) {
        percpu_data_start = .;

        *(.data.percpu)

        percpu_data_end = .;
    }

    .bss : AT(ADDR(.bss) - VIRTUAL_BASE) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
    }

    .init.text ALIGN (0x1000) : AT(ADDR(.init.text) - VIRTUAL_BASE) {
        init_mem_start = . - VIRTUAL_BASE;

        *(.init.text)
    }

    .init.data ALIGN (0x1000) : AT(ADDR(.init.data) - VIRTUAL_BASE) {
        *(.init.data)
    }

    .init.call ALIGN (0x1000) : AT(ADDR(.init.call) - VIRTUAL_BASE) {
        initcall_start = .;

        INITCALL_SECTION(0)
        INITCALL_SECTION(1)
        INITCALL_SECTION(2)
        INITCALL_SECTION(3)
        INITCALL_SECTION(4)
        INITCALL_SECTION(5)
        INITCALL_SECTION(6)
        INITCALL_SECTION(7)
        INITCALL_SECTION(8)
        INITCALL_SECTION(9)
        INITCALL_SECTION(10)

        initcall_end = .;
    }

    .init.param ALIGN (0x1000) : AT(ADDR(.init.param) - VIRTUAL_BASE) {
        param_start = .;

        *(.init.param)

        param_end = .;
    }

    init_mem_end = . - VIRTUAL_BASE;

    image_end = . - VIRTUAL_BASE;

    /DISCARD/ : {
        *(.comment)
        *(.note.gnu.build-id)
    }
}
