#include <stdbool.h>

#include "init/multiboot.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/math.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "log/log.h"

void breakpoint_triggered() {
}

typedef struct {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} PACKED elf_section_header_t;

extern uint32_t strtab_start;
extern uint32_t strtab_end;
extern uint32_t symtab_start;
extern uint32_t symtab_end;

static const char *strtab;
static uint32_t strtabsz;
static const elf_symbol_t *symtab;
static uint32_t symtabsz;

const char * debug_symbol_name(const elf_symbol_t *symbol) {
    if(symbol == NULL) return NULL;
    return (const char *) ((uint32_t) strtab + symbol->name);
}

const elf_symbol_t * debug_lookup_symbol(uint32_t address) {
    if(!strtabsz || !symtabsz) return NULL;

    for(uint32_t i = 0; i < (symtabsz / sizeof(elf_symbol_t)); i++) {
        if (ELF32_ST_TYPE(symtab[i].info) == ELF_TYPE_FUNC && address >= symtab[i].value && address < symtab[i].value + symtab[i].size) {
            return &symtab[i];
        }
    }

    return NULL;
}

static inline uint32_t get_address(thread_t *me, uint32_t vaddr, uint32_t off) {
    if(me && !virt_is_valid(me, (void *) vaddr)) {
        return 0;
    }
    return ((uint32_t *) vaddr)[off];
}

void dump_stack_trace(console_t *t) {
    uint32_t top, bot;

    thread_t *me = percpu_up ? current : NULL;
    if(me) {
        top = (uint32_t) me->kernel_stack_top;
        bot = (uint32_t) me->kernel_stack_bottom;
        vram_putsf(t, "Current task: %s (%u) 0x%X-0x%X\n", me->node->argv[0], me->node->pid, top, bot);
    } else {
        vram_puts(t, "Current task: NULL\n");
        top = 0;
        bot = (uint32_t) -1;
    }

    vram_puts(t, "Stack trace:\n");
    uint32_t last_ebp = 0, ebp, eip;
    asm("mov %%ebp, %0" : "=r" (ebp));

    if(me && (ebp < top || ebp > bot)) {
        me = NULL;
        vram_putsf(t, "Not on correct stack! (ebp=%X eip=%X) salvaging...\n", ebp, get_address(NULL, ebp, 1));
    }

    eip = get_address(me, ebp, 1);

    for(uint32_t frame = 0; eip && ebp && ebp > last_ebp && (!me || (ebp >= top && ebp <= bot)) && frame < MAX_STACK_FRAMES; frame++) {
        const elf_symbol_t *symbol = debug_lookup_symbol(eip - 1);
        if(symbol) {
            vram_putsf(t, "    %s+0x%X/0x%X\n", debug_symbol_name(symbol), eip - 1 - symbol->value, eip - 1);
        } else {
            vram_putsf(t, "    0x%X\n", eip - 1);
        }

        last_ebp = ebp;
        ebp = get_address(me, ebp, 0);
        eip = get_address(me, ebp, 1);
    }

    vram_puts(t, "Stack trace end\n");
}

void __init debug_remap() {
    if(strtabsz && symtabsz) {
        strtab = map_pages((phys_addr_t) strtab, DIV_UP(strtabsz, PAGE_SIZE));
        symtab = map_pages((phys_addr_t) symtab, DIV_UP(symtabsz, PAGE_SIZE));
    }
}

void __init debug_init() {
    elf_section_header_t *sh = (elf_section_header_t *) mbi->u.elf_sec.addr;

    for(uint32_t i = 0; i < mbi->u.elf_sec.num; i++) {
        const char *name = (const char *) sh[mbi->u.elf_sec.shndx].addr + sh[i].name;
        if (!strcmp(name, ".strtab")) {
            strtab = (const char *) sh[i].addr;
            strtabsz = sh[i].size;
        } else if (!strcmp(name, ".symtab")) {
            symtab = (elf_symbol_t *) sh[i].addr;
            symtabsz = sh[i].size;
        }
    }

    if(strtabsz) {
        kprintf("debug - strtab is present (0x%X:0x%X)", strtab, strtab + strtabsz);
    } else {
        kprintf("debug - strtab is not present");
    }

    if(symtabsz) {
        kprintf("debug - symtab is present (0x%X:0x%X)", symtab, symtab + symtabsz);
    } else {
        kprintf("debug - symtab is not present");
    }
}

phys_addr_t __init debug_kernel_start(phys_addr_t raw) {
    if(strtab) {
        raw = MIN(raw, (phys_addr_t) strtab);
    }
    if(symtab) {
        raw = MIN(raw, (phys_addr_t) symtab);
    }
    return raw;
}

phys_addr_t __init debug_kernel_end(phys_addr_t raw) {
    if(strtab) {
        raw = MAX(raw, (phys_addr_t) (strtab + strtabsz));
    }
    if(symtab) {
        raw = MAX(raw, (phys_addr_t) (symtab + symtabsz));
    }
    return raw;
}
