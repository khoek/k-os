include ../Makefile.config

.PHONY: all clean doc

SRCDIR := src
INCDIR := inc
OBJDIR := build

ASFLAGS := --32

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -I$(INCDIR) \
	-std=gnu99 -fno-strict-aliasing -nostdlib -ffreestanding -c -g \
	-include "common/config.h"

LDFLAGS := -m32 -nostdlib -ffreestanding -g -Wl,--build-id=none

ASSRCS := $(shell find $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

CCSRCS := $(shell find $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(CCOBJS)

LDSCRIPTSRC := $(SRCDIR)/linker.ld.S
LDSCRIPT := $(OBJDIR)/linker.ld

all: $(KBINARY)

$(ASOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(AS) $(ASFLAGS) -o $@ $<

$(CCOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CCFLAGS) -o $@ $<

$(LDSCRIPT): $(LDSCRIPTSRC)
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(CPP) -I$(INCDIR) -D__LINKER__ -P -o $@ $<

$(KBINARY): $(OBJS) $(LDSCRIPT)
	@echo "      ld  $@"
	@$(CC) $(LDFLAGS) -T $(LDSCRIPT) -o $@ $(OBJS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(KBINARY)

doc:
	@echo "running doxygen"
	@doxygen
