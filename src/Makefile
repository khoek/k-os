-include ../Makefile.local

ASFLAGS=--32

CCFLAGS=-Wall -Wextra -Werror -m32 -I../inc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c

OBJS=	loader.o string.o printf.o rand.o kmain.o debug.o panic.o console.o \
        log.o gdt.o registers.o idt.o isr.o module.o mm.o cache.o pit.o \
        tsc.o syscall.o keyboard.o pci.o ide.o ahci.o vfs.o task.o cpl.o \
        binfmt/binfmt.o binfmt/binfmt_elf.o shell.o erasure_tool.o

all: kernel.elf

.s.o:
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

linker.ld: linker.ld.S
	@echo "      pp  $<"
	@$(CPP) -I../inc -D__LINKER__ -P linker.ld.S -o linker.ld

kernel.elf: $(OBJS) linker.ld
	@echo "      ld"
	@$(LD) -T linker.ld -o kernel.elf $(OBJS)

clean:
	rm -f *.o
	rm -f linker.ld
	rm -f kernel.elf

.PHONY: all clean
