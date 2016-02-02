include Makefile.config

UTILDIR = $(realpath util)
UTILBINDIR = $(UTILDIR)/$(UTILBINPREFIX)
LIBDIR = $(realpath libc)
KERNELDIR = $(realpath kernel)
APPSDIR = $(realpath apps)
DISTDIR = $(realpath dist)

export UTILDIR UTILBINDIR LIBDIR KERNELDIR APPSDIR DISTDIR

.PHONY: kernel libc util apps dist run
.NOTPARALLEL:

all: kernel libc util apps dist

kernel:
	@make -C kernel all

libc:
	@make -C libc all

util:
	@make -C util all

apps: libc util
	@make -C apps all

dist: kernel apps
	@make -C dist all

clean:
	@make -C util clean
	@make -C libc clean
	@make -C kernel clean
	@make -C apps clean
	@make -C dist clean

$(HDD):
	@echo "      dd  $(HDD)"
	@dd if=/dev/zero of=$(HDD) count=64000 bs=1k 2>&1
	@parted $(HDD) -s -- mklabel msdos > /dev/null 2>&1
	@parted $(HDD) -s -- mkpartfs primary fat32 1 -0 > /dev/null 2>&1

run: dist $(HDD)
	@echo "      qemu"
	@$(QEMU) -smp cores=8 -boot d -cdrom $(DISTDIR)/$(IMAGE) -drive id=disk,file=$(HDD),if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0
