#include "init.h"

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")

ENTRY(loader)
SECTIONS {
    . = 0x00120000;

    .text ALIGN (0x1000) : {
        *(.text)
    }

    .rodata ALIGN (0x1000) : {
        *(.rodata*)
    }

    .data ALIGN (0x1000) : {
        *(.data)
    }

    .bss : {
        sbss = .;
        *(COMMON)
        *(.bss)
        ebss = .;
    }

    .init.text ALIGN (0x1000) : {
        init_text_start = .;

        *(.init.text)

        init_text_end = .;
    }

    .init.data ALIGN (0x1000) : {
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

    .end_of_image : {
        end_of_image = .;
    }
}
