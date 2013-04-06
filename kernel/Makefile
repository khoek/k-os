-include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -I../libk/inc -Iinc -std=gnu99 \
		-nostdlib -nostartfiles -nodefaultlibs -ffreestanding

INCS=	-I../libk/inc -Iinc

OBJS=	src/loader.o src/kmain.o src/panic.o src/console.o \
		src/gdt.o src/gdthelper.o \
		src/idt.o src/idthelper.o \
		src/mm.o src/mmhelper.o src/malloc.o \
		src/pit.o src/rand.o src/keyboard.o \
		src/pci.o src/ide.o src/ahci.o src/vfs.o \
                src/shell.o src/erasure_tool.o

.s.o:
	@echo "    nasm  $<"
	@$(NASM) -f elf -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(GCC) $(CFLAGS) $(INCS) -c -o $@ $<

kernel.a: $(OBJS)
	@echo "building kernel.a"
	@$(AR) rcs kernel.a $(OBJS)

../build/kernel.elf: kernel.a
	@echo "linking kernel"
	@$(LD) -T ../linker.ld -o ../build/kernel.elf kernel.a -L../libk -lk

clean:
	rm -f ./*/*.o
	rm -f kernel.a

.PHONY: clean
