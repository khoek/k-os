include ../Makefile.config

.PHONY: all clean

LIBDIR ?= ../libc
UTILBINDIR ?= ../util/bin

LIBCLD := linker.ld

RESDIR := res
SRCDIR := src
OUTDIR := build

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -std=gnu99 \
        -nostdlib -ffreestanding -I$(LIBDIR)/inc -g -O3 -fno-strict-aliasing

RESS := $(shell find $(RESDIR) -type f -name "*")
RESO := $(patsubst $(RESDIR)/%,$(OUTDIR)/%,$(RESS))

SRCS := $(shell find $(SRCDIR) -type f -name "*.c")
OBJS := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%,$(SRCS))
LIBS := $(LIBDIR)/$(LIBCAR)

all: $(ROOTRAMFS)

$(ROOTRAMFS): $(OBJS) $(RESO)
	@$(UTILBINDIR)/$(MKROOTRAMFS) -o $@ $(OUTDIR)
	@echo "      mkrootramfs"

$(OBJS): $(OUTDIR)/% : $(SRCDIR)/%.c $(LIBS)
	@echo "      cc  $(patsubst $(SRCDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CCFLAGS) -T $(LIBDIR)/$(LIBCLD) -o $@ $< $(LIBS)

$(RESO): $(OUTDIR)/% : $(RESDIR)/%
	@echo "      cp  $(patsubst $(RESDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

clean:
	rm -rf $(OUTDIR)
	rm -rf $(ROOTRAMFS)
