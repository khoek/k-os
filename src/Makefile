-include ../Makefile.local

ASFLAGS=--32

CCFLAGS=-Wall -Wextra -Werror -m32 -I. -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c

HEADERS=debug.h elf.h task.h printf.h shell.h io.h mm.h panic.h version.h \
        console.h log.h cache.h ahci.h init.h module.h string.h common.h \
        erasure_tool.h vfs.h pci.h multiboot.h rand.h idt.h keyboard.h pit.h \
        tsc.h ide.h gdt.h syscall.h registers.h

OBJS=	loader.o string.o printf.o rand.o kmain.o elf.o panic.o console.o \
        log.o gdt.o registers.o idt.o isr.o module.o mm.o cache.o pit.o \
        tsc.o syscall.o keyboard.o pci.o ide.o ahci.o vfs.o task.o shell.o \
        erasure_tool.o

all: kernel.elf

.s.o:
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c $(HEADERS)
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

linker.ld: linker.ld.S $(HEADERS)
	@echo "      pp  $<"
	@$(CPP) -D__LINKER__ -P linker.ld.S -o linker.ld

kernel.elf: $(OBJS) linker.ld
	@echo "      ld"
	@$(LD) -T linker.ld -o kernel.elf $(OBJS)

clean:
	rm -f *.o
	rm -f linker.ld
	rm -f kernel.elf

.PHONY: all clean
