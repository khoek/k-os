#include <stdbool.h>

#include "lib/int.h"
#include "lib/string.h"
#include "common/math.h"
#include "init/initcall.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "sched/sched.h"
#include "sched/task.h"
#include "fs/binfmt.h"
#include "fs/elf.h"
#include "video/log.h"

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

static int load_elf_exe(const char *name, void *start, uint32_t length) {
    uint8_t *file = start;
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) start;

    if(!elf_header_valid(ehdr)) return -1;
    if(ehdr->e_phoff == 0) return -1;

    task_t *task = task_create(name, false, (void *) ehdr->e_entry, (void *) (0x10000 + PAGE_SIZE));
    alloc_page_user(0, task, 0x10000); //alloc stack, FIXME hardcoded, might overlap with an elf segment
    Elf32_Phdr *phdr = (Elf32_Phdr *) (file + ehdr->e_phoff);
    for(uint32_t i = 0; i < ehdr->e_phnum; i++) {
        switch(phdr[i].p_type) { //TODO validate the segments as we are going
            case PT_NULL:
                break;
            case PT_LOAD: {
                volatile uint32_t data_len = MIN(phdr[i].p_filesz, phdr[i].p_memsz);
                uint32_t zero_len = phdr[i].p_memsz - data_len;

                uint32_t data_coppied = 0;
                uint32_t zeroes_written = 0;

                uint32_t addr = phdr[i].p_vaddr;
                while(data_coppied < data_len) {
                    uint8_t *new_page = alloc_page_user(0, task, addr);

                    uint32_t data_left = data_len - data_coppied;
                    uint32_t chunk_len = MIN(data_left, PAGE_SIZE);

                    memcpy(new_page, start + phdr[i].p_offset + data_coppied, chunk_len);

                    data_coppied += chunk_len;
                    data_left -= chunk_len;

                    //zero the rest of the page
                    if(!data_left) {
                        uint32_t zero_chunk_len = PAGE_SIZE - chunk_len;

                        memset(new_page + chunk_len, 0, zero_chunk_len);
                        zeroes_written += zero_chunk_len;
                    }

                    //TODO unmap the mapped page

                    addr += PAGE_SIZE;
                }

                while(zeroes_written < zero_len) {
                    void *new_page = alloc_page_user(0, task, addr);

                    //zero the whole page for security
                    memset(new_page, 0, PAGE_SIZE);

                    zeroes_written += PAGE_SIZE;

                    //TODO unmap the mapped page

                    addr += PAGE_SIZE;
                }
                break;
            }
            case PT_TLS: {
                break;
            }
            default:
                kprintf("binfmt_elf - unsupported phdr type (0x%X)", phdr[i].p_type);
                continue;
        }
    }

    task_add(task);

    return 0;
}

static binfmt_t elf = {
    .load_exe = load_elf_exe,
};

static INITCALL elf_register_binfmt() {
    binfmt_register(&elf);

    return 0;
}

core_initcall(elf_register_binfmt);
