#include "common.h"
#include "init.h"
#include "binfmt.h"

int load_elf_exe(file_t UNUSED(*file)) {
    return 1;
}

int load_elf_lib(file_t UNUSED(*file)) {
    return 1;
}

binfmt_t elf = {
    .load_exe = load_elf_exe,
    .load_lib = load_elf_lib
};

INITCALL elf_register_binfmt() {
    binfmt_register(&elf);

    return 0;
}

core_initcall(elf_register_binfmt);
