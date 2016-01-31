OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(_start)
SECTIONS {
    . = 0x500000;

    .text ALIGN (0x1000) : AT(ADDR(.text)) {
        *(.text.entry)
        *(.text.startup)
        *(.text)
        *(.rodata*)
    }
    _data_start = .;
    .data ALIGN (0x1000) : AT(ADDR(.data)) {
        *(.data)
    }
    .bss ALIGN (0x1000) : AT(ADDR(.bss)) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
    }
    _data_end = .;
    /DISCARD/ : {
        *(.comment)
    }
}
