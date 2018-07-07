ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all extras clean

.DELETE_ON_ERROR:

EXTRASDIR := extras
RESDIR := res
SRCDIR := src
OUTDIR := build

CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -std=gnu99 -g -O3

RESS := $(shell find -L $(RESDIR) -type f -name "*")
RESO := $(patsubst $(RESDIR)/%,$(OUTDIR)/%,$(RESS))

SRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
OBJS := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%,$(SRCS))

EXTRA_FLAG_DIST_BUILT := $(EXTRASDIR)/.flag.dist-built

all: $(ROOTRAMFS)

extras:
	$(MAKE) -C $(EXTRASDIR) all

$(EXTRA_FLAG_DIST_BUILT):
	$(MAKE) extras

$(ROOTRAMFS): $(RESO) $(OBJS) $(EXTRA_FLAG_DIST_BUILT)
	@echo "      mkrootramfs"
	@$(UTILBINDIR)/$(MKROOTRAMFS) -o $@ $(OUTDIR)

$(RESO): $(OUTDIR)/% : $(RESDIR)/%
	@echo "      cp  $(patsubst $(RESDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

$(OBJS): $(OUTDIR)/% : $(SRCDIR)/%.c $(KSYSLIBDIR)/libc.a
	@echo "      cc  $(patsubst $(SRCDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CFLAGS) -o $@ $<

clean-all: clean
	$(MAKE) -C $(EXTRASDIR) clean-all

clean-all-noredl: clean
	$(MAKE) -C $(EXTRASDIR) clean

clean:
	rm -rf $(OUTDIR)
	rm -rf $(ROOTRAMFS)
