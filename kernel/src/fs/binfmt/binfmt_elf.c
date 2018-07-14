#include <stdbool.h>

#include "common/types.h"
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

#define USTACK_ADDR_START 0xB0000000
#define UARGS_ADDR_START 0xB8000000

#define USTACK_NUM_PAGES 8
#define UARGS_NUM_PAGES 1

#define USTACK_SIZE (USTACK_NUM_PAGES * PAGE_SIZE)

#define designate_stack(t)                                  \
    designate_space(t, (void *) USTACK_ADDR_START, USTACK_NUM_PAGES)

#define designate_args(t)                                   \
    designate_space(t, (void *) UARGS_ADDR_START, UARGS_NUM_PAGES)

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

static void * designate_space(thread_t *t, void *start, uint32_t num_pages) {
    while(resolve_virt(t, start)) {
        panic("WRITING TO ALREADY ALLOC'D SPACE!");
        //start += num_pages * PAGE_SIZE;
    }

    page_t *page = alloc_pages(num_pages, ALLOC_ZERO);
    user_map_pages(current, start, page_to_phys(page), num_pages);

    return start;
}

static uint32_t calculate_strtablen(char **tab) {
    if(!tab) return 0;

    uint32_t tablen = 0;
    while(*(tab++)) {
        tablen++;
    }
    return tablen;
}

uint32_t strcpylen(char *dest, const char *src) {
    uint32_t len = 0;
    while ((*dest++ = *src++) != '\0') {
        len++;
    }

    return len;
}

//returns a pointer to just after the copied strtab
static void * build_strtab(char **start, char **tab, uint32_t *out_tablen) {
    uint32_t tablen = calculate_strtablen(tab);
    if(out_tablen) {
        *out_tablen = tablen;
    }

    char *end = (void *) (start + tablen * sizeof(char *) + 1);
    while(*tab) {
        *(start++) = end;
        end += strcpylen(end, *tab++) + 1;
    }
    *start = NULL;

    return end;
}

static int32_t load_elf(binary_t *binary) {
    uint32_t flags;
    irqsave(&flags);

    void *olddir = arch_replace_mem(current, NULL);

    Elf32_Ehdr *ehdr = kmalloc(sizeof(Elf32_Ehdr));
    if(vfs_seek(binary->file, 0, SEEK_SET) != 0) {
        goto fail_io;
    }
    if(vfs_read(binary->file, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        //TODO check for error, and in this case report fail_io instead
        goto fail_not_elf;
    }

    if(!elf_header_valid(ehdr)) goto fail_not_elf;
    if(!ehdr->e_phoff) goto fail_not_elf;

    kprintf("binfmt_elf - phdr @ %X", ehdr->e_phoff);

    Elf32_Phdr *phdr = kmalloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if(vfs_seek(binary->file, ehdr->e_phoff, SEEK_SET) != ehdr->e_phoff) {
        goto fail_io;
    }
    if(vfs_read(binary->file, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum)
        != ((ssize_t) (sizeof(Elf32_Phdr) * ehdr->e_phnum))) {
        goto fail_io;
    }

    for(uint32_t i = 0; i < ehdr->e_phnum; i++) {
        // TODO: validate the segments as we are going, like, everything is
        // vulnerable!
        switch(phdr[i].p_type) {
            case PT_NULL:
                break;
            case PT_LOAD: {
                kprintf("binfmt_elf - LOAD (%X, %X) @ %X -> %X - %X", phdr[i].p_filesz, phdr[i].p_memsz, phdr[i].p_offset, phdr[i].p_vaddr, phdr[i].p_vaddr + phdr[i].p_memsz);

                void *addr = (void *) phdr[i].p_vaddr;
                if(vfs_seek(binary->file, phdr[i].p_offset, SEEK_SET)
                    != phdr[i].p_offset) {
                    goto fail_io;
                }

                uint32_t bytes_written = 0;
                while(bytes_written < phdr[i].p_memsz) {
                    if(!user_get_page(current, addr + bytes_written)) {
                        user_alloc_page(current, addr + bytes_written, 0);
                    }

                    uint32_t page_bytes_left = PAGE_SIZE - (((uint32_t) (addr + bytes_written)) % PAGE_SIZE);

                    int32_t chunk_len;
                    if(bytes_written < phdr[i].p_filesz) {
                        chunk_len = MIN(page_bytes_left, phdr[i].p_filesz - bytes_written);
                        if(vfs_read(binary->file, addr + bytes_written, chunk_len) != chunk_len) {
                            goto fail_io;
                        }
                    } else {
                        chunk_len = MIN(page_bytes_left, phdr[i].p_memsz - bytes_written);
                        memset(addr + bytes_written, 0, chunk_len);
                    }

                    bytes_written += chunk_len;
                }

                break;
            }
            case PT_TLS: {
                panicf("binfmt_elf - unsupported phdr TLS");
                break;
            }
            default:
                panicf("binfmt_elf - unsupported phdr type (0x%X)", phdr[i].p_type);
                break;
        }
    }

    kprintf("binfmt_elf - entry: %X", ehdr->e_entry);

    irqdisable();

    thread_t *me = current;

    task_node_t *node = obtain_task_node(me);

    //an irqsave guards this entire function
    spin_lock(&node->lock);
    node->argv = binary->argv;
    node->envp = binary->envp;
    spin_unlock(&node->lock);

    void *ustack = designate_stack(me) + USTACK_SIZE;

    uint32_t argc;
    void *arg_buff = designate_args(me);

    void *uargv = arg_buff;
    arg_buff = build_strtab(arg_buff, binary->argv, &argc);

    void *uenvp = arg_buff;
    arg_buff = build_strtab(arg_buff, binary->envp, NULL);

    pl_bootstrap_userland((void *) ehdr->e_entry, ustack, argc, uargv, uenvp);

    BUG();

fail_not_elf:
    arch_replace_mem(current, olddir);

    irqstore(flags);
    return -ENOEXEC;

fail_io:
    arch_replace_mem(current, olddir);

    irqstore(flags);
    return -EIO;
}

static binfmt_t elf = {
    .load = load_elf,
};

static INITCALL elf_register_binfmt() {
    binfmt_register(&elf);

    return 0;
}

core_initcall(elf_register_binfmt);
