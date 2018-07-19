#include "init/initcall.h"
#include "arch/bios.h"
#include "mm/mm.h"

uint8_t *bios_bda_ptr = (void *) BIOS_BDA;

void __init bios_early_remap() {
    bios_bda_ptr = map_page(BIOS_BDA);
}
