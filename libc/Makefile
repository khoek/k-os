-include ../Makefile.local

ASFLAGS=--32 -g

CCFLAGS=-Wall -g -Wextra -Werror -Wno-unused-parameter -m32 -I../inc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c

OBJS=	src/crt.o src/syscall.o

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
	/usr/local/cross/bin/i586-elf-gcc -std=gnu99 -c -Iinc -o main.o main.c
	/usr/local/cross/bin/i586-elf-ld -T linker.ld -o main.mod src/syscall.o src/crt.o main.o
	cp main.mod ../kernel/build/
	make -C ../kernel run

.PHONY: all clean
