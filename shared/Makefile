ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

RESDIR := res
BUILDDIR := build

INCS := syscall_decls.h syscall_ents.h
KERNELSRCS :=
LIBKSRCS := syscall_hooks.S

RESSRCS := $(shell find -L $(RESDIR) -type f -name "*")
OBJS := $(addprefix $(BUILDDIR)/$(SHAREDINCPREFIX)/shared/, $(INCS)) $(addprefix $(BUILDDIR)/$(SHAREDSRCPREFIX)/kernel/shared/, $(KENRELSRCS)) $(addprefix $(BUILDDIR)/$(SHAREDSRCPREFIX)/libk/shared/, $(LIBKSRCS))

all: $(OBJS)

$(RESSRCS): $(BUILDDIR)/% : $(RESDIR)/%
	@echo "      cp  $< $@"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

$(BUILDDIR)/inc/shared/syscall_decls.h: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makedecls.py $< $@

$(BUILDDIR)/inc/shared/syscall_ents.h: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makeents.py $< $@

$(BUILDDIR)/src/libk/shared/syscall_hooks.S: data/syscalls.txt
	@echo "      py  $(patsubst $(SRCDIR)/%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@python scripts/makehooks.py $< $@

clean:
	rm -rf $(BUILDDIR)
