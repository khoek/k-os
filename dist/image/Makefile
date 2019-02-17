ROOTPATH ?= $(shell readlink -f ../..)
include $(ROOTPATH)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

SRCDIR := root
OBJDIR := build

SRCS := $(shell find -L $(SRCDIR) -type f -name "*")
OBJS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SRCS))

all: $(DISTPKGDIR)/$(IMAGE)

$(OBJDIR)/$(ROOTRAMFS): $(APPSPATH)/$(ROOTRAMFS)
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

$(OBJDIR)/$(KBINARY): $(KERNELPATH)/$(KBINARY)
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

$(OBJS): $(OBJDIR)/% : $(SRCDIR)/%
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

$(DISTPKGDIR)/$(IMAGE): $(OBJS) $(OBJDIR)/$(ROOTRAMFS) $(OBJDIR)/$(KBINARY)
	@echo "      mkimage"
	@mkdir -p $(shell dirname $@)
	@grub-mkrescue -o $@ $(OBJDIR)

clean:
	rm -rf $(DISTPKGDIR)
	rm -rf $(OBJDIR)
