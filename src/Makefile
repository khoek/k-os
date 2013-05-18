-include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -m32 -I. -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding

OBJS=	loader.o string.o printf.o rand.o \
        kmain.o elf.o panic.o console.o \
        gdt.o registers.o idt.o isr.o \
        module.o mm.o cache.o \
        pit.o syscall.o keyboard.o \
        pci.o ide.o ahci.o vfs.o \
        shell.o erasure_tool.o \

all: kernel.elf

.s.o:
	@echo "      as  $<"
	@$(AS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS) linker.ld
	@echo "linking kernel"
	@echo "      ld"
	@$(LD) -T linker.ld -o kernel.elf $(OBJS)

clean:
	rm -f *.o
	rm -f kernel.elf

.PHONY: all clean

