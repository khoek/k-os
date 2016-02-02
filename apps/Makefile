include ../Makefile.config

.PHONY: all clean

LIBDIR ?= ../libc
UTILBINDIR ?= ../util/bin

SRCDIR := src
OBJDIR := build

CCFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -m32 -std=gnu99 \
        -nostdlib -ffreestanding -I$(LIBDIR)/inc -g -O3 -fno-strict-aliasing -T $(LIBDIR)/linker.ld

SRCS := $(shell find $(SRCDIR) -type f -name "*.c")
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%,$(SRCS))
LIBS := $(LIBDIR)/$(LIBCAR)

all: $(ROOTRAMFS)

$(ROOTRAMFS): $(OBJS)
	@$(UTILBINDIR)/$(MKROOTRAMFS) -o $@ $(OBJDIR)
	@echo "      mkrootramfs"

$(OBJS): $(OBJDIR)/% : $(SRCDIR)/%.c
	@echo "      cc  $(patsubst $(SRCDIR)%,%,$<) -> $(patsubst $(OBJDIR)%,%,$@)"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CCFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(ROOTRAMFS)
