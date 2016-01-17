#include "init/initcall.h"
#include "arch/bios.h"
#include "mm/mm.h"

uint8_t *bios_bda_ptr;

static INITCALL bios_init() {
    bios_bda_ptr = map_page((void *) BIOS_BDA);
    return 0;
}

early_initcall(bios_init);
