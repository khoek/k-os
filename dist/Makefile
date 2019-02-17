ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

.PHONY: all clean clean-image clean-run clean-all

.DELETE_ON_ERROR:

TESTDIR := test

all:
	$(MAKE) -C $(DISTIMAGEDIR) all

$(TESTDIR)/$(TESTHDD):
	@echo "      dd  $(TESTDIR)/$(TESTHDD)"
	@mkdir -p $(shell dirname $@)
	@dd if=/dev/zero of=$(TESTDIR)/$(TESTHDD) count=64000 bs=1k 2>&1
	@parted $(TESTDIR)/$(TESTHDD) -s -- mklabel msdos > /dev/null 2>&1
	@parted $(TESTDIR)/$(TESTHDD) -s -- mkpartfs primary fat32 1 -0 > /dev/null 2>&1

run: $(TESTDIR)/$(TESTHDD)
	@echo "      qemu"
	@$(QEMU) $(QEMUARGS)

debug: $(TESTDIR)/$(TESTHDD)
	@echo "      qemu"
	@$(QEMU) -s -S $(QEMUARGS)

clean: clean-image clean-run

clean-image:
	$(MAKE) -C $(DISTIMAGEDIR) clean

clean-run:
	rm -rf $(TESTDIR)

clean-all: clean
	rm -rf $(KSYSROOT)
