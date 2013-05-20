#include "init.h"

ENTRY (loader)

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
        SECTION_INITCALL_LEVEL(INITCALL)
        initcall_end = .;

        init_data_end = .;
    }

    .end_of_image : {
        end_of_image = .;
    }
}
