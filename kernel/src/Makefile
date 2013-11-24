-include ../Makefile.local

ASFLAGS=--32

CCFLAGS=-Wall -Wextra -Werror -Wno-unused-parameter -m32 -I../inc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c -g

OBJS=	loader.o \
        init/kmain.o init/param.o \
        lib/string.o lib/printf.o lib/rand.o \
        bug/debug.o bug/panic.o \
        sync/semaphore.o \
        video/console.o video/log.o \
        arch/proc.o arch/gdt.o arch/registers.o arch/idt.o arch/isr.o arch/acpi.o arch/mp.o arch/pit.o arch/tsc.o arch/cpl.o arch/atomic.o arch/spinlock.o \
        mm/mm.o mm/cache.o \
        time/clock.o time/timer.o\
        device/device.o \
        sched/sched.o sched/proc.o sched/task.o sched/syscall.o \
        net/interface.o net/packet.o \
        net/socket.o \
        net/eth/eth.o \
        net/ip/af_inet.o net/ip/ip.o net/ip/arp.o net/ip/icmp.o net/ip/tcp.o net/ip/udp.o net/ip/dhcp.o net/ip/nbns.o net/ip/raw.o \
        fs/module.o fs/fd.o fs/vfs.o fs/block.o fs/subblock.o fs/disk.o \
        fs/label/msdos.o fs/label/gpt.o \
        fs/file/char.o \
        fs/type/devfs.o fs/type/ramfs.o \
        fs/binfmt/binfmt.o fs/binfmt/binfmt_elf.o \
        driver/bus/pci.o \
        driver/input/keyboard.o \
        driver/disk/ide.o driver/disk/ahci.o \
        driver/net/e1000.o \
        misc/stats.o misc/sysrq.o

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
