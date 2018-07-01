ROOTDIR ?= $(shell readlink -f .)/..
include $(ROOTDIR)/Makefile.config

.PHONY: all clean doc

SRCDIR := src
INCDIR := inc
OBJDIR := build
DOCDIR := doc

ASFLAGS := --32

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter           \
	-Wno-missing-field-initializers -m32 -I$(INCDIR)               \
	-std=gnu99 -fno-strict-aliasing -nostdlib -ffreestanding -c -g \
	-include "common/config.h"

LDFLAGS := -m32 -nostdlib -ffreestanding -g -Wl,--build-id=none

ASSRCS := $(shell find -L $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

CCSRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(CCOBJS)

LDSCRIPTSRC := $(SRCDIR)/arch/linker.ld.S
LDSCRIPT := $(OBJDIR)/arch/linker.ld

all: $(KBINARY)

$(ASOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KAS) $(ASFLAGS) -o $@ $<

$(CCOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CCFLAGS) -DKERNEL_SOURCE -o $@ $<

$(LDSCRIPT): $(LDSCRIPTSRC)
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCPP) -I$(INCDIR) -D__LINKER__ -P -o $@ $<

$(KBINARY): $(OBJS) $(LDSCRIPT)
	@echo "      ld  $@"
	@$(KCC) $(LDFLAGS) -T $(LDSCRIPT) -o $@ $(OBJS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(DOCDIR)
	rm -rf $(KBINARY)

doc:
	@echo "running doxygen"
	@doxygen
