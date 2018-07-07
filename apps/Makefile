ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

RESDIR := res
SRCDIR := src
OUTDIR := build

CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -std=gnu99 -g -O3

RESS := $(shell find -L $(RESDIR) -type f -name "*")
RESO := $(patsubst $(RESDIR)/%,$(OUTDIR)/%,$(RESS))

EXTRAS := $(shell mkdir -p $(APPSEXTRAPREFIX); find -L $(APPSEXTRAPREFIX) -type f -name "*")
EXTRAO := $(patsubst $(APPSEXTRAPREFIX)/%,$(OUTDIR)/%,$(EXTRAS))

SRCS := $(shell find -L $(SRCDIR) -type f -name "*.c")
OBJS := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%,$(SRCS))

all: $(ROOTRAMFS)

$(ROOTRAMFS): $(RESO) $(EXTRAO) $(OBJS)
	@echo "      mkrootramfs"
	@$(UTILBINDIR)/$(MKROOTRAMFS) -o $@ $(OUTDIR)

$(RESO): $(OUTDIR)/% : $(RESDIR)/%
	@echo "      cp  $(patsubst $(RESDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

$(EXTRAO): $(OUTDIR)/% : $(APPSEXTRAPREFIX)/%
	@echo "      cp  $(patsubst $(APPSEXTRAPREFIX)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@cp $< $@

$(OBJS): $(OUTDIR)/% : $(SRCDIR)/%.c $(KSYSLIBDIR)/libc.a
	@echo "      cc  $(patsubst $(SRCDIR)%,%,$<) -> $(patsubst $(OUTDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@$(KCC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(APPSEXTRAPREFIX)
	rm -rf $(OUTDIR)
	rm -rf $(ROOTRAMFS)
