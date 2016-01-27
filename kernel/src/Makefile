-include ../Makefile.local

ASFLAGS:=--32

CCFLAGS:=-Wall -Wextra -Werror -Wno-unused-parameter -m32 -I../inc -std=gnu99 \
	-include "common/config.h" \
	-nostdlib -ffreestanding -c -g -O3 -fno-strict-aliasing

LDFLAGS:=-m32 -T linker.ld -o kernel.elf -nostdlib -ffreestanding -g -O3 \
	-Wl,--build-id=none

SRCS:= $(shell find . -type f -name "*.s")
SRCS+= $(shell find . -type f -name "*.c")

OBJS:= $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(SRCS)))

all: kernel.elf

%.o: %.s
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

linker.ld: linker.ld.S
	@echo "      pp  $<"
	@$(CPP) -I../inc -D__LINKER__ -P linker.ld.S -o linker.ld

kernel.elf: $(OBJS) linker.ld
	@echo "      ld  $@"
	@$(CC) $(LDFLAGS) $(OBJS)

clean:
	find . -name \*.o -type f -delete
	rm -f linker.ld
	rm -f kernel.elf

.PHONY: all clean
