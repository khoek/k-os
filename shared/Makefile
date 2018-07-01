ROOTDIR ?= $(shell readlink -f .)/..
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

BUILDDIR := build

INCS := syscall_decls.h syscall_ents.h
SRCS := syscall_hooks.S

OBJS := $(addprefix $(BUILDDIR)/$(SHAREDINCPREFIX)/shared/, $(INCS)) $(addprefix $(BUILDDIR)/$(SHAREDSRCPREFIX)/shared/, $(SRCS))

all: $(OBJS)

$(BUILDDIR)/inc/shared/syscall_decls.h: res/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makedecls.py $< $@

$(BUILDDIR)/inc/shared/syscall_ents.h: res/syscalls.txt
	@echo "     sed  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makeents.py $< $@

$(BUILDDIR)/src/shared/syscall_hooks.S: res/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makehooks.py $< $@

clean:
	rm -rf $(BUILDDIR)
