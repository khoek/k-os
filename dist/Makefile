include ../Makefile.config

.PHONY: all clean

KERNELDIR ?= ../kernel
APPSDIR ?= ../apps

SRCDIR := root
OBJDIR := build

SRCS := $(shell find $(SRCDIR) -type f -name "*")
OBJS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SRCS))

all: $(IMAGE)

$(OBJDIR)/$(ROOTRAMFS): $(APPSDIR)/$(ROOTRAMFS)
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

$(OBJDIR)/$(KBINARY): $(KERNELDIR)/$(KBINARY)
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

$(OBJS): $(OBJDIR)/% : $(SRCDIR)/%
	@echo "      cp  $< -> $@"
	@mkdir -p $(shell dirname $@)
	@cp -r $< $@

cdrom.iso: $(OBJS) $(OBJDIR)/$(ROOTRAMFS) $(OBJDIR)/$(KBINARY)
	@echo "      mkimage"
	@grub-mkrescue -o $(IMAGE) $(OBJDIR) > /dev/null 2>&1

clean:
	rm -rf $(OBJDIR)
	rm -rf $(IMAGE)
