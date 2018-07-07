ROOTDIR ?= $(shell readlink -f ../..)
include $(ROOTDIR)/Makefile.config

DLDIR := dl
SRCDIR := src
BUILDDIR := build
PATCHDIR := patch

export CFLAGS := -D_POSIX_VERSION=200809L -DNO_TTY_DRIVER=1 -g

export CC := $(TOOLCHAINBINDIR)/bin/$(TARGET)-gcc
export AR := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ar
export AS := $(TOOLCHAINBINDIR)/bin/$(TARGET)-as
export LD := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ld
export NM := $(TOOLCHAINBINDIR)/bin/$(TARGET)-nm
export RANLIB := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ranlib
export STRIP := $(TOOLCHAINBINDIR)/bin/$(TARGET)-strip
export OBJCOPY := $(TOOLCHAINBINDIR)/bin/$(TARGET)-objcopy
export OBJDUMP := $(TOOLCHAINBINDIR)/bin/$(TARGET)-objdump
export READELF := $(TOOLCHAINBINDIR)/bin/$(TARGET)-readelf

WGETFLAGS := -c

BASH_VER ?= 4.4
COREUTILS_VER ?= 8.30

NAME_BASH := bash-$(BASH_VER)
NAME_COREUTILS := coreutils-$(COREUTILS_VER)

AR_BASH := $(NAME_BASH).tar.gz
AR_COREUTILS := $(NAME_COREUTILS).tar.xz

URL_BASH := https://ftp.gnu.org/gnu/bash/$(AR_BASH)
URL_COREUTILS := https://ftp.gnu.org/gnu/coreutils/$(AR_COREUTILS)

ARTIFACT_BASH := $(BUILDDIR)/$(NAME_BASH)/bash
ARTIFACT_COREUTILS := fakecore

PATCH_BASH := $(PATCHDIR)/$(NAME_BASH).patch

PUBLISHED_BASH := $(APPSDIR)/build/sbin/bash

FLAG_DIST_BUILT := .flag.dist-built

.PHONY: all required publish dl clean-all clean clean-dl clean-src clean-build clean-noredl

.DELETE_ON_ERROR:

#$(ARTIFACT_COREUTILS)
all: publish
	@touch $(FLAG_DIST_BUILT)

$(FLAG_DIST_BUILT):
	$(MAKE) all

required: $(FLAG_DIST_BUILT)

dl: $(DLDIR)/$(AR_BASH) $(DLDIR)/$(AR_COREUTILS)

$(DLDIR)/$(AR_BASH):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_BASH)

$(DLDIR)/$(AR_COREUTILS):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_COREUTILS)

$(SRCDIR)/$(NAME_BASH)/configure: $(DLDIR)/$(AR_BASH)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_BASH)
	touch $@

$(SRCDIR)/$(NAME_COREUTILS)/configure: $(DLDIR)/$(AR_COREUTILS)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_COREUTILS)
	touch $@

.flag.$(NAME_BASH)-patched: $(SRCDIR)/$(NAME_BASH)/configure
	patch -p0 < $(PATCH_BASH)
	touch $@

$(BUILDDIR)/$(NAME_BASH)/Makefile: .flag.$(NAME_BASH)-patched
	mkdir -p $(BUILDDIR)/$(NAME_BASH)
	cd $(APPSDIR)/extras/$(BUILDDIR)/$(NAME_BASH) && sh $(APPSDIR)/extras/$(SRCDIR)/$(NAME_BASH)/configure -v --build=$(shell gcc -dumpmachine) --host=$(TARGET)
	cd $(APPSDIR)/extras

$(BUILDDIR)/$(NAME_COREUTILS)/Makefile: $(SRCDIR)/$(NAME_COREUTILS)/configure
	mkdir -p $(BUILDDIR)/$(NAME_COREUTILS)
	cd $(APPSDIR)/extras/$(BUILDDIR)/$(NAME_COREUTILS) && sh $(APPSDIR)/extras/$(SRCDIR)/$(NAME_COREUTILS)/configure -v --build=$(shell gcc -dumpmachine) --host=$(TARGET)
	cd $(APPSDIR)/extras

$(ARTIFACT_BASH): $(BUILDDIR)/$(NAME_BASH)/Makefile
	$(MAKE) -C $(shell dirname $<)

$(ARTIFACT_COREUTILS): $(BUILDDIR)/$(NAME_COREUTILS)/Makefile
	$(MAKE) -C $(shell dirname $<)

publish: $(PUBLISHED_BASH)

$(PUBLISHED_BASH): $(ARTIFACT_BASH)
	@mkdir -p $(shell dirname $@)
	cp $(ARTIFACT_BASH) $(PUBLISHED_BASH)

clean-all: clean-dl clean-src clean-build clean-dist

clean: clean-noredl

clean-noredl: clean-src clean-build clean-dist

clean-dl:
	rm -rf $(DLDIR)

clean-src:
	rm -rf $(SRCDIR)
	rm -f .flag.$(NAME_BASH)-patched
	rm -f .flag.$(NAME_COREUTILS)-patched .flag.$(NAME_COREUTILS)-reconfigured

clean-build:
	rm -rf $(BUILDDIR)

clean-dist:
	rm -f $(FLAG_DIST_BUILT)
