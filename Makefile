ROOTDIR ?= $(shell readlink -f .)
include Makefile.config

.PHONY: all toolchain clean clean-toolchain run debug shared kernel libc util apps dist run

all: kernel libc util apps dist

complete: toolchain
	$(MAKE) all

toolchain:
	+$(MAKE) -C toolchain all

util:
	$(MAKE) -C util all

shared: util
	$(MAKE) -C shared all

kernel: shared
	$(MAKE) -C kernel all

libc: shared
	$(MAKE) -C libc all

apps: libc util
	$(MAKE) -C apps all

dist: kernel apps
	$(MAKE) -C dist all

clean:
	@rm -rf $(HDD)
	$(MAKE) -C shared clean
	$(MAKE) -C util clean
	$(MAKE) -C kernel clean
	$(MAKE) -C libc clean
	$(MAKE) -C apps clean
	$(MAKE) -C dist clean

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
