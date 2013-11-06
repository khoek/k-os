#include "init/initcall.h"

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

ENTRY(entry)
SECTIONS {
    . = PHYSICAL_BASE;

    .image_start : AT(ADDR(.image_start) - VIRTUAL_BASE) {
        image_start = .;
    }

    boot_mem_start = . + VIRTUAL_BASE;

    .header : {
       *(.header)
    }

    .entry.ap ALIGN (0x1000) : {
        *(.entry.ap)
    }

    .entry : {
       *(.entry)
    }

    boot_mem_end = . + VIRTUAL_BASE;

    . += VIRTUAL_BASE;

    .text ALIGN (0x1000) : AT(ADDR(.text) - VIRTUAL_BASE) {
        *(.text)
        *(.rodata*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - VIRTUAL_BASE) {
        *(.data)
    }

    .bss : AT(ADDR(.bss) - VIRTUAL_BASE) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
    }

    . -= VIRTUAL_BASE;

    init_mem_start = .;

    . += VIRTUAL_BASE;

    .init.text ALIGN (0x1000) : AT(ADDR(.init.text) - VIRTUAL_BASE) {
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

        initcall_end = .;
    }

    .init.param ALIGN (0x1000) : AT(ADDR(.init.param) - VIRTUAL_BASE) {
        param_start = .;

        *(.init.param)

        param_end = .;
    }

    . -= VIRTUAL_BASE;

    init_mem_end = .;

    .image_end : AT(ADDR(.image_end) - VIRTUAL_BASE) {
        image_end = .;
    }

    /DISCARD/ : {
        *(.comment)
    }
}
