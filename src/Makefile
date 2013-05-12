-include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -m32 -I. -std=gnu99 \
		-nostdlib -nostartfiles -nodefaultlibs -ffreestanding

OBJS=	string.o printf.o \
		loader.o kmain.o \
		panic.o \
		console.o \
		gdt.o gdthelper.o \
		idt.o idthelper.o \
		mm.o mmhelper.o \
		pit.o rand.o keyboard.o \
		pci.o ide.o ahci.o vfs.o \
		shell.o erasure_tool.o \

all: kernel.elf
		
.s.o:
	@echo "    nasm  $<"
	@$(NASM) -f elf -g -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CFLAGS) -c -g -o $@ $<

kernel.elf: $(OBJS) linker.ld
	@echo "linking kernel"
	@echo "      ld"
	@$(LD) -T linker.ld -g -o kernel.elf $(OBJS)

clean:
	rm -f *.o
	rm -f kernel.elf

.PHONY: all clean
