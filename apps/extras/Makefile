ROOTDIR ?= $(shell readlink -f ../..)
include $(ROOTDIR)/Makefile.config

DLDIR := dl
SRCDIR := src
BUILDDIR := build
BINDIR := dist
PATCHDIR := patch

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

CFLAGS_BASH := -D_POSIX_VERSION=200809L -DNO_TTY_DRIVER=1 -g
CFLAGS_COREUTILS := -D_POSIX_VERSION=200809L

NAME_BASH := bash-$(BASH_VER)
NAME_COREUTILS := coreutils-$(COREUTILS_VER)

AR_BASH := $(NAME_BASH).tar.gz
AR_COREUTILS := $(NAME_COREUTILS).tar.xz

URL_BASH := https://ftp.gnu.org/gnu/bash/$(AR_BASH)
URL_COREUTILS := https://ftp.gnu.org/gnu/coreutils/$(AR_COREUTILS)

ARTIFACT_BASH := $(BUILDDIR)/$(NAME_BASH)/bash
ARTIFACT_COREUTILS := $(BUILDDIR)/$(NAME_COREUTILS)/src/coreutils

PATCH_BASH := $(PATCHDIR)/$(NAME_BASH).patch
PATCH_COREUTILS := $(PATCHDIR)/$(NAME_COREUTILS).patch

PUBLISHED_BASH := $(APPSDIR)/build/bin/bash
PUBLISHED_COREUTILS := $(APPSDIR)/build/bin/coreutils

FLAG_DIST_BUILT := .flag.dist-built

.PHONY: all required publish dl clean-all clean clean-dl clean-src clean-build clean-noredl

.DELETE_ON_ERROR:

#$(ARTIFACT_COREUTILS)
all: publish
	@touch $(FLAG_DIST_BUILT)

$(FLAG_DIST_BUILT): $(KSYSLIBDIR)/libk.a $(KSYSLIBDIR)/libc.a
	$(MAKE) all

required: $(FLAG_DIST_BUILT)

dl: $(DLDIR)/$(AR_BASH) $(DLDIR)/$(AR_COREUTILS)

$(DLDIR)/$(AR_BASH):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_BASH)

$(DLDIR)/$(AR_COREUTILS):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_COREUTILS)

$(SRCDIR)/$(NAME_BASH)/configure: $(DLDIR)/$(AR_BASH) $(KSYSLIBDIR)/libk.a $(KSYSLIBDIR)/libc.a
	rm -rf $(BUILDDIR)/$(NAME_BASH)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_BASH)
	touch $@

$(SRCDIR)/$(NAME_COREUTILS)/configure: $(DLDIR)/$(AR_COREUTILS) $(KSYSLIBDIR)/libk.a $(KSYSLIBDIR)/libc.a
	rm -rf $(BUILDDIR)/$(NAME_COREUTILS)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_COREUTILS)
	touch $@

.flag.$(NAME_BASH)-patched: $(SRCDIR)/$(NAME_BASH)/configure
	patch -p0 < $(PATCH_BASH)
	touch $@

.flag.$(NAME_COREUTILS)-patched: $(SRCDIR)/$(NAME_COREUTILS)/configure
	patch -p0 < $(PATCH_COREUTILS)
	touch $@

$(BUILDDIR)/$(NAME_BASH)/Makefile: .flag.$(NAME_BASH)-patched $(DLDIR)/$(AR_BASH) $(KSYSLIBDIR)/libk.a
	mkdir -p $(BUILDDIR)/$(NAME_BASH)
	cd $(APPSDIR)/extras/$(BUILDDIR)/$(NAME_BASH) && export CFLAGS="$(CFLAGS_BASH)" && sh $(APPSDIR)/extras/$(SRCDIR)/$(NAME_BASH)/configure -v --build=$(shell gcc -dumpmachine) --host=$(TARGET) --prefix=/
	cd $(APPSDIR)/extras

$(BUILDDIR)/$(NAME_COREUTILS)/Makefile: .flag.$(NAME_COREUTILS)-patched $(DLDIR)/$(AR_BASH) $(KSYSLIBDIR)/libk.a
	mkdir -p $(BUILDDIR)/$(NAME_COREUTILS)
	mkdir -p $(APPSDIR)/extras/$(BINDIR)/$(NAME_COREUTILS)
	cd $(APPSDIR)/extras/$(BUILDDIR)/$(NAME_COREUTILS) && export CFLAGS="$(CFLAGS_COREUTILS)" && sh $(APPSDIR)/extras/$(SRCDIR)/$(NAME_COREUTILS)/configure -v --build=$(shell gcc -dumpmachine) --host=$(TARGET) --prefix=/ --enable-single-binary --without-selinux --without-gmp --enable-no-install-program=stdbuf
	cd $(APPSDIR)/extras

$(ARTIFACT_BASH): $(BUILDDIR)/$(NAME_BASH)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) DESTDIR=$(APPSDIR)/extras/$(BINDIR)/$(NAME_BASH) install

$(ARTIFACT_COREUTILS): $(BUILDDIR)/$(NAME_COREUTILS)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) DESTDIR=$(APPSDIR)/extras/$(BINDIR)/$(NAME_COREUTILS) install

publish: $(PUBLISHED_BASH) $(PUBLISHED_COREUTILS)

$(PUBLISHED_BASH): $(ARTIFACT_BASH)
	@mkdir -p $(shell dirname $@)
	cp $(ARTIFACT_BASH) $(PUBLISHED_BASH)

$(PUBLISHED_COREUTILS): $(ARTIFACT_COREUTILS)
	@mkdir -p $(APPSDIR)/build/bin
	-cp -r $(BINDIR)/$(NAME_COREUTILS)/bin/. $(APPSDIR)/build/bin

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
	rm -rf $(BINDIR)
