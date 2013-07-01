#include <stdbool.h>

#include "int.h"
#include "string.h"
#include "common.h"
#include "debug.h"
#include "init.h"
#include "binfmt.h"
#include "elf.h"
#include "mm.h"
#include "task.h"
#include "log.h"

static bool elf_header_valid(Elf32_Ehdr *ehdr) {
    return ehdr->e_ident[EI_MAG0] == ELFMAG0
        && ehdr->e_ident[EI_MAG1] == ELFMAG1
        && ehdr->e_ident[EI_MAG2] == ELFMAG2
        && ehdr->e_ident[EI_MAG3] == ELFMAG3

        && ehdr->e_ident[EI_CLASS] == ELFCLASS32
        && ehdr->e_ident[EI_DATA]  == ELFDATA2LSB

        && ehdr->e_ident[EI_VERSION] == EV_CURRENT
        && ehdr->e_version           == EV_CURRENT

        && ehdr->e_machine == EM_386;
}

static int load_elf_exe(void *start, uint32_t length) {
    start = mm_map(start);
    for(uint32_t mapped = PAGE_SIZE; mapped < length; mapped += PAGE_SIZE) {
        mm_map((void *) (((uint32_t) start) + mapped));
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) start;

    if(!elf_header_valid(ehdr)) return -1;
    if(ehdr->e_phoff == 0) return -1;

    task_t *task = task_create(true, (void *) ehdr->e_entry, (void *) (0x10000 + PAGE_SIZE - 1));
    alloc_page_user(0, task, 0x10000); //alloc stack

    Elf32_Phdr *phdr = (Elf32_Phdr *) (((uint32_t) start) + ehdr->e_phoff);
    for(uint32_t i = 0; i < ehdr->e_phnum; i++) {
        switch(phdr[i].p_type) { //TODO validate the segments as we are going
            case PT_NULL:
                break;
            case PT_LOAD: {
                uint32_t page_num = 0;
                uint32_t bytes_left = phdr[i].p_memsz;
                while(bytes_left) {
                    void *new_page = alloc_page_user(0, task, phdr[i].p_vaddr + (page_num * PAGE_SIZE));
                    page_num++;

                    memcpy(new_page, (void *) (((uint32_t) start) + phdr[i].p_offset), MIN(bytes_left, PAGE_SIZE));
                    bytes_left -= MIN(bytes_left, PAGE_SIZE);
                }
                break;
            }
            default: //Unsupported phdr
                logf("UNSUP %u", phdr[i].p_type);
                return -1;
        }
    }

    task_schedule(task);

    return 0;
}

static int load_elf_lib(void UNUSED(*start), uint32_t UNUSED(length)) {
    return 1;
}

static binfmt_t elf = {
    .load_exe = load_elf_exe,
    .load_lib = load_elf_lib
};

static INITCALL elf_register_binfmt() {
    binfmt_register(&elf);

    return 0;
}

core_initcall(elf_register_binfmt);
