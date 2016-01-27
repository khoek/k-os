-include ../Makefile.local

ASFLAGS:=--32

CCFLAGS:=-Wall -Wextra -Werror -Wno-unused-parameter -m32 -Iinc -std=gnu99 \
        -nostdlib -ffreestanding --sysroot=inc -c -g -O3 -fno-strict-aliasing

LDFLAGS:=-nostdlib -g -O3 -melf_i386

SRCS:= $(shell find src -type f -name "*.s")
SRCS+= $(shell find src -type f -name "*.c")

OBJS:= $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(SRCS)))

all: libc.a init

.s.o:
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

libc.a: $(OBJS)
	@echo "      ar  $@"
	@rm -f libc.a
	@$(AR) rcs libc.a $(OBJS)

init: libc.a init.o
	@echo "      ld  init.o"
	@$(LD) $(LDFLAGS) -o init init.o libc.a

clean:
	find . -name \*.o -type f -delete
	rm -f libc.a
	rm -f init

.PHONY: all clean
