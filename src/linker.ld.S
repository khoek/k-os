#include "init.h"

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

ENTRY(loader)
SECTIONS {
    . = PHYSICAL_BASE;

    .image_start : AT(ADDR(.image_start) - VIRTUAL_BASE) {
        image_start = .;
    }

    .header : {
       *(.header)
    }

    .loader : {
       *(.loader)
    }

    . += VIRTUAL_BASE;

    .text : AT(ADDR(.text) - VIRTUAL_BASE) {
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

    .init.text ALIGN (0x1000) : AT(ADDR(.init.text) - VIRTUAL_BASE) {
        init_text_start = .;

        *(.init.text)

        init_text_end = .;
    }

    .init.data ALIGN (0x1000) : AT(ADDR(.init.data) - VIRTUAL_BASE) {
        init_data_start = .;

        *(.init.data)

        initcall_start = .;
        INITCALL_SECTION(0)
        INITCALL_SECTION(1)
        INITCALL_SECTION(2)
        INITCALL_SECTION(3)
        INITCALL_SECTION(4)
        INITCALL_SECTION(5)
        INITCALL_SECTION(6)
        INITCALL_SECTION(7)
        initcall_end = .;

        init_data_end = .;
    }

    . -= VIRTUAL_BASE;

    .image_end : AT(ADDR(.image_end) - VIRTUAL_BASE) {
        image_end = .;
    }

    /DISCARD/ : {
        *(.comment)
    }
}
