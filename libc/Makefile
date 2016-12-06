ROOTDIR ?= $(shell readlink -f .)/..
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

SRCDIR := src
INCDIR := inc
OBJDIR := build

ASFLAGS := --32

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -I$(INCDIR) \
		-std=gnu99 -nostdlib -ffreestanding --sysroot=$(INCDIR) -c -g -O3 -fno-strict-aliasing

ASSRCS := $(shell find $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

CCSRCS := $(shell find $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(CCOBJS)

all: $(LIBCAR)

$(ASOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KAS) $(ASFLAGS) -o $@ $<

$(CCOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CCFLAGS) -o $@ $<

$(LIBCAR): $(OBJS)
	@echo "      ar  $@"
	@rm -f $@
	@$(KAR) rcs $@ $(OBJS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBCAR)
