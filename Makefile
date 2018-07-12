ROOTPATH ?= $(shell readlink -f .)
include Makefile.config

.PHONY: all complete aux aux-required toolchain-required libc-required toolchain run debug shared kernel libk libc util apps dist run clean-all clean-all-noredl clean clean-run clean-aux clean-libc clean-toolchain

.DELETE_ON_ERROR:

all: dist

complete: aux
	$(MAKE) all

aux:
	$(MAKE) toolchain
	$(MAKE) libc

aux-required: libc-required

dl: toolchain-dl libc-dl

toolchain:
	$(MAKE) -C toolchain all
toolchain-required:
	$(MAKE) -C toolchain required
toolchain-dl:
	$(MAKE) -C toolchain dl

libc: toolchain-required shared
	$(MAKE) -C libc all
libc-required: toolchain-required shared
	$(MAKE) -C libc required
libc-dl:
	$(MAKE) -C libc dl

util:
	$(MAKE) -C util all

shared: util
	$(MAKE) -C shared all

kernel: aux-required shared
	$(MAKE) -C kernel all

libk: aux-required shared
	$(MAKE) -C libk all

apps: aux-required libk util
	$(MAKE) -C apps all

dist: kernel apps
	$(MAKE) -C dist all

clean-all: clean-run
	$(MAKE) -C toolchain clean-all
	$(MAKE) -C libc clean-all
	$(MAKE) -C shared clean
	$(MAKE) -C util clean
	$(MAKE) -C kernel clean
	$(MAKE) -C libk clean
	$(MAKE) -C apps clean-all
	$(MAKE) -C dist clean-all

clean-all-noredl: clean-run
	$(MAKE) -C toolchain clean
	$(MAKE) -C libc clean
	$(MAKE) -C shared clean
	$(MAKE) -C util clean
	$(MAKE) -C kernel clean
	$(MAKE) -C libk clean
	$(MAKE) -C apps clean-all-noredl
	$(MAKE) -C dist clean-all

clean: clean-run
	$(MAKE) -C shared clean
	$(MAKE) -C util clean
	$(MAKE) -C kernel clean
	$(MAKE) -C libk clean
	$(MAKE) -C apps clean
	$(MAKE) -C dist clean

clean-run:
	@rm -rf $(HDD)

clean-aux: clean-libc clean-toolchain

clean-libc:
	$(MAKE) -C libc clean

clean-toolchain:
	$(MAKE) -C toolchain clean

$(HDD):
	@echo "      dd  $(HDD)"
	@dd if=/dev/zero of=$(HDD) count=64000 bs=1k 2>&1
	@parted $(HDD) -s -- mklabel msdos > /dev/null 2>&1
	@parted $(HDD) -s -- mkpartfs primary fat32 1 -0 > /dev/null 2>&1

run: dist $(HDD)
	@echo "      qemu"
	@$(QEMU) $(QEMUARGS)

debug: dist $(HDD)
	@echo "      qemu"
	@$(QEMU) -s -S $(QEMUARGS)
