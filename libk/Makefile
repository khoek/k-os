ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

SRCDIR := src
INCDIR := inc
OBJDIR := build

ASFLAGS := -m32 -I$(INCDIR) -nostdlib -ffreestanding -c

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -I$(INCDIR) \
	-I$(LIBCBINDIR)/$(TARGET)/include																			\
	-std=gnu99 -nostdlib -ffreestanding --sysroot=$(INCDIR) 							\
	--sysroot=$(LIBCBINDIR)/$(TARGET)/include -c -g -O3

ASSRCS := $(shell find -L $(SRCDIR) -type f -name "*.s")
ASOBJS := $(patsubst $(SRCDIR)/%.s,$(OBJDIR)/%.o,$(ASSRCS))

ASPPSRCS := $(shell find -L $(SRCDIR) -type f -name "*.S")
ASPPOBJS := $(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.o,$(ASPPSRCS))

CCSRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
CCOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CCSRCS))

OBJS := $(ASOBJS) $(ASPPOBJS) $(CCOBJS)

all: $(LIBKAR)

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
	@$(KCC) $(CCFLAGS) -DLIBK_SOURCE -o $@ $<

$(LIBKAR): $(OBJS)
	@echo "      ar  $@"
	@rm -f $@
	@$(KAR) rcs $@ $(OBJS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(LIBKAR)
