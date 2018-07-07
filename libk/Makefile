ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

SRCDIR := src
INCDIR := inc
OBJDIR := build

LIBDIR := $(LIBKBINDIR)/lib
CRTDIR := $(LIBKBINDIR)/crt

CRTOBJS := $(CRTDIR)/crt0.o

ASFLAGS := -m32 -I$(INCDIR) -nostdlib -ffreestanding -c

CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -std=gnu99 -c -g -O3 \
 	-nostdlib -ffreestanding

ASSRCS := $(shell find -L $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

ASPPSRCS := $(shell find -L $(SRCDIR) -type f -name "*.S")
ASPPOBJS := $(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.o,$(ASPPSRCS))

CCSRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(ASPPOBJS) $(CCOBJS)

all: publish

$(CRTOBJS): $(CRTDIR)/%.o : $(OBJDIR)/%.o
	@mkdir -p $(shell dirname $@)
	cp $< $@

$(ASOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.s
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(ASFLAGS) -o $@ $<

$(ASPPOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.S
	@echo "      as  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(ASFLAGS) -o $@ $<

$(CCOBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)/%,%,$<)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CFLAGS) -DLIBK_SOURCE -o $@ $<

$(LIBDIR)/$(LIBKAR): $(OBJS)
	@echo "      ar  $@"
	@rm -f $@
	@mkdir -p $(shell dirname $@)
	@$(KAR) rcs $@ $(OBJS)

$(KSYSLIBDIR)/$(LIBKAR): $(LIBDIR)/$(LIBKAR) $(CRTOBJS)
	@mkdir -p $(KSYSLIBDIR)
	cp -r $(LIBDIR)/. $(KSYSLIBDIR)
	cp -r $(CRTDIR)/. $(KSYSLIBDIR)

publish: $(KSYSLIBDIR)/$(LIBKAR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBKBINDIR)
