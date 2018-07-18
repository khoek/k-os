ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

.PHONY: all clean doc

.DELETE_ON_ERROR:

SRCDIR := src
INCDIR := inc
OBJDIR := build
DOCDIR := doc

ASFLAGS := --32

CFLAGS := -Wall -Wextra -Werror -Wno-missing-field-initializers \
  -Wno-unused-parameter -Wno-unused-function -Wno-unused-value                 \
	-m32 -I$(INCDIR) -std=gnu99 -fno-strict-aliasing -nostdlib -ffreestanding -c \
	-O3 -g -include "common/config.h"

LDFLAGS := -m32 -nostdlib -ffreestanding -g -O3 -Wl,--build-id=none

ASSRCS := $(shell find -L $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

CCSRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(CCOBJS)

LIBS := -static-libgcc -lgcc

LDSCRIPTSRC := $(SRCDIR)/arch/linker.ld.pp
LDSCRIPT := $(OBJDIR)/arch/linker.ld

all: $(KBINARY)

$(ASOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KAS) $(ASFLAGS) -o $@ $<

$(CCOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CFLAGS) -DKERNEL_SOURCE -o $@ $<

$(LDSCRIPT): $(LDSCRIPTSRC)
	@echo "      pp  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCPP) -I$(INCDIR) -DLINKER_SOURCE -P -o $@ $<

$(KBINARY): $(OBJS) $(LDSCRIPT)
	@echo "      ld  $@"
	@$(KCC) $(LDFLAGS) -T $(LDSCRIPT) -o $@ $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(DOCDIR)
	rm -rf $(KBINARY)

doc:
	@echo "running doxygen"
	@doxygen
