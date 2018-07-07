ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

DLDIR := $(LIBCDIR)/dl
SRCDIR := $(LIBCDIR)/src
PATCHDIR := $(LIBCDIR)/patch
BUILDDIR := $(LIBCDIR)/build

export CC_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-gcc
export AR_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ar
export AS_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-as
export LD_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ld
export NM_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-nm
export RANLIB_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-ranlib
export STRIP_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-strip
export OBJCOPY_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-objcopy
export OBJDUMP_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-objdump
export READELF_FOR_TARGET := $(TOOLCHAINBINDIR)/bin/$(TARGET)-readelf

WGETFLAGS := -c

# These versions CANNOT be higher: automake requires perl version <= 5.22,
# and newlib (currently, 3.0.0) needs these versions of automake and
# autoconf.
NAME_NEWLIB := newlib-$(NEWLIB_VER)

AR_NEWLIB := $(NAME_NEWLIB).tar.gz

URL_NEWLIB := ftp://sourceware.org/pub/newlib/$(AR_NEWLIB)

ARTIFACT_NEWLIB := $(LIBCBINDIR)/$(TARGET)/lib/libc.a

PATCH_NEWLIB := $(PATCHDIR)/$(NAME_NEWLIB).patch

PUBLISHED_NEWLIB := $(KSYSLIBDIR)/libc.a

export AUTOCONF := $(TOOLCHAINBINDIR)/bin/autoconf
export AUTORECONF := $(TOOLCHAINBINDIR)/bin/autoreconf
export AUTOMAKE := $(TOOLCHAINBINDIR)/bin/automake
export ACLOCAL := $(TOOLCHAINBINDIR)/bin/aclocal

FLAG_DIST_BUILT := .flag.dist-built

.PHONY: all required dl libgcc publish clean-all clean clean-dl clean-src clean-build clean-noredl

.DELETE_ON_ERROR:

all: publish libgcc
	@touch $(FLAG_DIST_BUILT)

$(FLAG_DIST_BUILT):
	$(MAKE) all

required: $(FLAG_DIST_BUILT)

dl: $(DLDIR)/$(AR_NEWLIB)

$(DLDIR)/$(AR_NEWLIB):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_NEWLIB)

$(SRCDIR)/$(NAME_NEWLIB)/configure: $(DLDIR)/$(AR_NEWLIB)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_NEWLIB)
	touch $@

#TODO ADD SOME CONSTANDS FOR THESE COMMANDS
.flag.$(NAME_NEWLIB)-patched: $(SRCDIR)/$(NAME_NEWLIB)/configure
	rm -rf $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos
	cp -r sys $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos
	mkdir -p $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos/include
	cp -r $(LIBKDIR)/inc/. $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos/include
	patch -p0 < $(PATCH_NEWLIB)
	touch $@

.flag.$(NAME_NEWLIB)-reconfigured: .flag.$(NAME_NEWLIB)-patched
	(cd $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys; $(AUTOCONF))
	(cd $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos; $(ACLOCAL) -I ../../.. && $(AUTOCONF) && $(AUTOMAKE) --cygnus --add-missing Makefile)
	#(cd $(SRCDIR)/$(NAME_NEWLIB)/newlib/libc/sys/kos; $(AUTORECONF))
	touch $@

$(BUILDDIR)/$(NAME_NEWLIB)/Makefile: .flag.$(NAME_NEWLIB)-reconfigured $(ARTIFACT_AUTOMAKE) $(ARTIFACT_AUTOCONF)
	mkdir -p $(LIBCBINDIR)
	mkdir -p $(BUILDDIR)/$(NAME_NEWLIB)
	cd $(BUILDDIR)/$(NAME_NEWLIB) && sh $(SRCDIR)/$(NAME_NEWLIB)/configure -v --build=$(shell gcc -dumpmachine) --target=$(TARGET) --prefix=$(LIBCBINDIR)
	cd $(LIBCDIR)

$(ARTIFACT_NEWLIB): $(BUILDDIR)/$(NAME_NEWLIB)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install
	cp -r -L $(LIBKDIR)/inc/. $(LIBCHEADERDIR)

#FIXME HACKY
#FIXME Clean this if we run make clean? surely...
libgcc: $(ARTIFACT_NEWLIB)
		$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(TOOLCHAINDIR) libgcc

publish: $(PUBLISHED_NEWLIB)

$(PUBLISHED_NEWLIB): $(ARTIFACT_NEWLIB)
	@mkdir -p $(KSYSLIBDIR)
	cp -r $(LIBCHEADERDIR)/. $(KSYSHEADERDIR)
	cp $(LIBCBINDIR)/$(TARGET)/lib/libc.a $(KSYSLIBDIR)
	cp $(LIBCBINDIR)/$(TARGET)/lib/libg.a $(KSYSLIBDIR)
	cp $(LIBCBINDIR)/$(TARGET)/lib/libm.a $(KSYSLIBDIR)

clean-all: clean-dl clean-src clean-build clean-dist

clean: clean-noredl

clean-noredl: clean-src clean-build clean-dist

clean-dl:
	rm -rf $(DLDIR)

clean-src:
	rm -rf $(SRCDIR)
	rm -f .flag.$(NAME_NEWLIB)-patched .flag.$(NAME_NEWLIB)-reconfigured

clean-build:
	rm -rf $(BUILDDIR)

clean-dist:
	rm -rf $(LIBCBINDIR)
	rm -f $(FLAG_DIST_BUILT)
