ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

BUILDDIR := build

INCS := syscall_decls.h syscall_ents.h
SRCS := syscall_hooks.S

OBJS := $(addprefix $(BUILDDIR)/$(SHAREDINCPREFIX)/shared/, $(INCS)) $(addprefix $(BUILDDIR)/$(SHAREDSRCPREFIX)/shared/, $(SRCS))

all: $(OBJS)
	cp -a res/inc/. $(BUILDDIR)/inc

$(BUILDDIR)/inc/shared/syscall_decls.h: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makedecls.py $< $@

$(BUILDDIR)/inc/shared/syscall_ents.h: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makeents.py $< $@

$(BUILDDIR)/src/shared/syscall_hooks.S: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makehooks.py $< $@

clean:
	rm -rf $(BUILDDIR)
