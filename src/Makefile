-include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -I. -std=gnu99 \
		-nostdlib -nostartfiles -nodefaultlibs -ffreestanding

OBJS=	string.o printf.o \
		loader.o kmain.o \
		panic.o \
		console.o \
		gdt.o gdthelper.o \
		idt.o idthelper.o \
		mm.o mmhelper.o malloc.o \
		pit.o rand.o keyboard.o \
		pci.o ide.o ahci.o vfs.o \
		shell.o erasure_tool.o \

all: kernel.elf
		
.s.o:
	@echo "    nasm  $<"
	@$(NASM) -f elf -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS)
	@echo "linking kernel"
	@$(LD) -T linker.ld -o kernel.elf $(OBJS)

clean:
	rm -f *.o

.PHONY: all clean
