ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

LIBKLD := linker.ld

RESDIR := res
SRCDIR := src
OUTDIR := build

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -std=gnu99 \
        -nostdlib -ffreestanding -I$(LIBKDIR)/inc -I$(LIBCBINDIR)/$(TARGET)/include -g -O3

RESS := $(shell find -L $(RESDIR) -type f -name "*")
RESO := $(patsubst $(RESDIR)/%,$(OUTDIR)/%,$(RESS))

SRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
OBJS := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%,$(SRCS))
LIBS := $(LIBKDIR)/libk.a $(LIBCBINDIR)/$(TARGET)/lib/libc.a $(LIBCBINDIR)/$(TARGET)/lib/libg.a $(LIBCBINDIR)/$(TARGET)/lib/libm.a $(LIBKDIR)/libk.a

# $(LIBKDIR)/$(LIBKAR)

all: $(ROOTRAMFS)

$(ROOTRAMFS): $(OBJS) $(RESO)
	@echo "      mkrootramfs"
	@$(UTILBINDIR)/$(MKROOTRAMFS) -o $@ $(OUTDIR)

$(OBJS): $(OUTDIR)/% : $(SRCDIR)/%.c $(LIBS)
	@echo "      cc  $(patsubst $(SRCDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CCFLAGS) -T $(LIBKDIR)/$(LIBKLD) -o $@ $< $(LIBS)

$(RESO): $(OUTDIR)/% : $(RESDIR)/%
	@echo "      cp  $(patsubst $(RESDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

clean:
	rm -rf $(OUTDIR)
	rm -rf $(ROOTRAMFS)
