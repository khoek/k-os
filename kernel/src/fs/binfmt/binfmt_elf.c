#include <stdbool.h>

#include "lib/int.h"
#include "lib/string.h"
#include "common/math.h"
#include "init/initcall.h"
#include "bug/debug.h"
#include "arch/pl.h"
#include "mm/mm.h"
#include "arch/proc.h"
#include "sched/task.h"
#include "fs/binfmt.h"
#include "fs/elf.h"
#include "log/log.h"

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

static bool load_elf(file_t *f) {
    Elf32_Ehdr *ehdr = kmalloc(sizeof(Elf32_Ehdr));
    if(vfs_read(f, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        return false;
    }

    if(!elf_header_valid(ehdr)) return false;
    if(!ehdr->e_phoff) return false;

    //alloc stack, FIXME hardcoded, might overlap with an elf segment
    user_map_page(current, (void *) 0x10000, page_to_phys(alloc_page(0)));

    kprintf("binfmt_elf - phdr @ %X", ehdr->e_phoff);

    Elf32_Phdr *phdr = kmalloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if(vfs_seek(f, ehdr->e_phoff) != ehdr->e_phoff) {
        return false;
    }
    if(vfs_read(f, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum)
        != ((ssize_t) (sizeof(Elf32_Phdr) * ehdr->e_phnum))) {
        return false;
    }

    for(uint32_t i = 0; i < ehdr->e_phnum; i++) {
        //TODO validate the segments as we are going
        switch(phdr[i].p_type) {
            case PT_NULL:
                break;
            case PT_LOAD: {
                kprintf("binfmt_elf - LOAD (%X, %X) @ %X -> %X", phdr[i].p_filesz, phdr[i].p_memsz, phdr[i].p_offset, phdr[i].p_vaddr);

                volatile uint32_t data_len = MIN(phdr[i].p_filesz, phdr[i].p_memsz);
                uint32_t zero_len = phdr[i].p_memsz - data_len;

                uint32_t data_coppied = 0;
                uint32_t zeroes_written = 0;

                void *addr = (void *) phdr[i].p_vaddr;
                if(vfs_seek(f, phdr[i].p_offset) != phdr[i].p_offset) {
                    return false;
                }

                while(data_coppied < data_len) {
                    page_t *page = alloc_page(0);
                    void *pagevirt = page_to_virt(page);
                    user_map_page(current, addr, page_to_phys(page));

                    uint32_t data_left = data_len - data_coppied;
                    ssize_t chunk_len = MIN(data_left, PAGE_SIZE);

                    if(vfs_read(f, pagevirt, chunk_len) != chunk_len) {
                        return false;
                    }

                    data_coppied += chunk_len;
                    data_left -= chunk_len;

                    //zero the rest of the page
                    if(!data_left) {
                        uint32_t zero_chunk_len = PAGE_SIZE - chunk_len;

                        memset(pagevirt + chunk_len, 0, zero_chunk_len);
                        zeroes_written += zero_chunk_len;
                    }

                    //TODO unmap the mapped page

                    addr += PAGE_SIZE;
                }

                while(zeroes_written < zero_len) {
                    //zero the whole page for security
                    page_t *page = alloc_page(ALLOC_ZERO);
                    user_map_page(current, addr, page_to_phys(page));

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

    kprintf("binfmt_elf - entry: %X", ehdr->e_entry);

    pl_enter_userland((void *) ehdr->e_entry, (void *) (0x10000 + PAGE_SIZE));

    return true;
}

static binfmt_t elf = {
    .load = load_elf,
};

static INITCALL elf_register_binfmt() {
    binfmt_register(&elf);

    return 0;
}

core_initcall(elf_register_binfmt);
