include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -I../libk/inc -Iinc -std=gnu99 \
		-nostdlib -nostartfiles -nodefaultlibs -ffreestanding

INCS=	-I../libk/inc -Iinc

OBJS=	src/loader.o src/kmain.o src/panic.o src/console.o \
		src/gdt.o src/gdthelper.o \
		src/idt.o src/idthelper.o \
		src/mm.o src/mmhelper.o src/malloc.o \
		src/pit.o src/rand.o src/keyboard.o \
		src/pci.o src/ide.o src/vfs.o \
                src/shell.o src/erasure_tool.o

.s.o:
	@echo "    nasm  $<"
	@nasm -f elf -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@i586-elf-gcc $(CFLAGS) $(INCS) -c -o $@ $<

kernel.a: $(OBJS)
	@echo "building kernel.a"
	@ar rcs kernel.a $(OBJS)

clean:
	rm -f ./*/*.o
	rm -f kernel.a

.PHONY: clean
