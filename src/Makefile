-include ../Makefile.local

ASFLAGS=--32 -g

CCFLAGS=-Wall -g -Wextra -Werror -Wno-unused-parameter -m32 -I../inc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c

OBJS=	loader.o \
        init/kmain.o init/module.o \
        lib/string.o lib/printf.o lib/rand.o \
        bug/debug.o bug/panic.o \
        video/console.o video/log.o \
        arch/gdt.o arch/registers.o arch/idt.o arch/isr.o arch/pit.o arch/tsc.o arch/cpl.o arch/atomic.o arch/spinlock.o \
        mm/mm.o mm/cache.o \
        time/clock.o \
        device/device.o \
        task/task.o task/syscall.o \
        net/ethernet.o \
        fs/vfs.o \
        fs/binfmt/binfmt.o fs/binfmt/binfmt_elf.o \
        driver/bus/pci.o \
        driver/input/keyboard.o \
        driver/disk/ide.o driver/disk/ahci.o \
        driver/net/e1000.o \
        misc/shell.o misc/erasure_tool.o

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
	find . -name \*.o -type f -delete
	rm -f linker.ld
	rm -f kernel.elf

.PHONY: all clean
