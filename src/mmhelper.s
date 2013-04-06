global paging_set_directory

paging_set_directory:
    ; page directory address:
    mov eax, [esp+4]
    mov cr3, eax
    ; enable paging
    mov eax, cr0
    or eax, (1 << 31) | (1 << 16) ; enable paging & kernel write protection
    mov cr0, eax
    ret
