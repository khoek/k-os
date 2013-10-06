-include ../Makefile.local

ASFLAGS=--32

CCFLAGS=-Wall -g -Wextra -Werror -Wno-unused-parameter -m32 -Iinc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding --sysroot=inc -c

OBJS=	src/crt.o src/syscall.o src/errno.o src/string.o src/log.o src/mm.o

all: libc.a

.s.o:
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

libc.a: $(OBJS)
	@echo "      ar  libc.a"
	@rm -f libc.a
	@$(AR) rcs libc.a $(OBJS)

clean:
	find . -name \*.o -type f -delete
	rm -f libc.a

test: all main.c
	make -C ../kernel
	rm -f ../kernel/build/kernel.elf
	rm -f ../kernel/build/main.mod
	rm -f ../kernel/build/song.mod
	/usr/local/cross/bin/i586-elf-gcc --sysroot=inc -std=gnu99 -c -Iinc -o main.o main.c
	/usr/local/cross/bin/i586-elf-gcc --sysroot=inc -std=gnu99 -c -Iinc -o song.o song.c
	/usr/local/cross/bin/i586-elf-ld -T linker.ld -o main.mod main.o libc.a
	/usr/local/cross/bin/i586-elf-ld -T linker.ld -o song.mod song.o libc.a
	cp main.mod ../kernel/build/
	cp song.mod ../kernel/build/
	make -C ../kernel run

.PHONY: all clean
