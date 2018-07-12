ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

SRCDIR := src
INCDIR := inc
OBJDIR := build

LIBDIR := $(LIBKBINPATH)/lib
CRTDIR := $(LIBKBINPATH)/crt

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

$(KSYSLIBPATH)/$(LIBKAR): $(LIBDIR)/$(LIBKAR) $(CRTOBJS)
	@mkdir -p $(KSYSLIBPATH)
	cp -r $(LIBDIR)/. $(KSYSLIBPATH)
	cp -r $(CRTDIR)/. $(KSYSLIBPATH)

publish: $(KSYSLIBPATH)/$(LIBKAR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBKBINPATH)
