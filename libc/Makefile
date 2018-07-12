ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

DLPATH := $(LIBCPATH)/dl
SRCPATH := $(LIBCPATH)/src
PATCHPATH := $(LIBCPATH)/patch
BUILDPATH := $(LIBCPATH)/build

export CC_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-gcc
export AR_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-ar
export AS_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-as
export LD_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-ld
export NM_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-nm
export RANLIB_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-ranlib
export STRIP_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-strip
export OBJCOPY_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-objcopy
export OBJDUMP_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-objdump
export READELF_FOR_TARGET := $(TOOLCHAINBINPATH)/bin/$(TARGET)-readelf

WGETFLAGS := -c

# These versions CANNOT be higher: automake requires perl version <= 5.22,
# and newlib (currently, 3.0.0) needs these versions of automake and
# autoconf.
NAME_NEWLIB := newlib-$(NEWLIB_VER)

AR_NEWLIB := $(NAME_NEWLIB).tar.gz

URL_NEWLIB := ftp://sourceware.org/pub/newlib/$(AR_NEWLIB)

ARTIFACT_NEWLIB := $(LIBCBINPATH)/$(TARGET)/lib/libc.a

PATCH_NEWLIB := $(PATCHPATH)/$(NAME_NEWLIB).patch

PUBLISHED_NEWLIB := $(KSYSLIBPATH)/libc.a

export AUTOCONF := $(TOOLCHAINBINPATH)/bin/autoconf
export AUTORECONF := $(TOOLCHAINBINPATH)/bin/autoreconf
export AUTOMAKE := $(TOOLCHAINBINPATH)/bin/automake
export ACLOCAL := $(TOOLCHAINBINPATH)/bin/aclocal

FLAG_DIST_BUILT := .flag.dist-built

.PHONY: all required dl libgcc publish clean-all clean clean-dl clean-src clean-build clean-noredl

.DELETE_ON_ERROR:

all: publish libgcc
	@touch $(FLAG_DIST_BUILT)

$(FLAG_DIST_BUILT):
	$(MAKE) all

required: $(FLAG_DIST_BUILT)

dl: $(DLPATH)/$(AR_NEWLIB)

$(DLPATH)/$(AR_NEWLIB):
	$(WGET) -P $(DLPATH) $(WGETFLAGS) $(URL_NEWLIB)

$(SRCPATH)/$(NAME_NEWLIB)/configure: $(DLPATH)/$(AR_NEWLIB)
	mkdir -p $(SRCPATH)
	$(TAR) -C $(SRCPATH) -xf $(DLPATH)/$(AR_NEWLIB)
	touch $@

#TODO ADD SOME CONSTANTS FOR THESE COMMANDS
.flag.$(NAME_NEWLIB)-patched: $(SRCPATH)/$(NAME_NEWLIB)/configure
	rm -rf $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys/kos
	cp -r sys $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys/kos
	mkdir -p $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys/kos/include
	cp -r $(LIBKPATH)/inc/. $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys/kos/include
	patch -p0 < $(PATCH_NEWLIB)
	touch $@

.flag.$(NAME_NEWLIB)-reconfigured: .flag.$(NAME_NEWLIB)-patched
	(cd $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys; $(AUTOCONF))
	(cd $(SRCPATH)/$(NAME_NEWLIB)/newlib/libc/sys/kos; $(ACLOCAL) -I ../../.. && $(AUTOCONF) && $(AUTOMAKE) --cygnus --add-missing Makefile)
	touch $@

$(BUILDPATH)/$(NAME_NEWLIB)/Makefile: .flag.$(NAME_NEWLIB)-reconfigured $(ARTIFACT_AUTOMAKE) $(ARTIFACT_AUTOCONF)
	mkdir -p $(LIBCBINPATH)
	mkdir -p $(BUILDPATH)/$(NAME_NEWLIB)
	cd $(BUILDPATH)/$(NAME_NEWLIB) && sh $(SRCPATH)/$(NAME_NEWLIB)/configure -v --build=$(shell gcc -dumpmachine) --target=$(TARGET) --prefix=$(LIBCBINPATH)
	cd $(LIBCPATH)

$(ARTIFACT_NEWLIB): $(BUILDPATH)/$(NAME_NEWLIB)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install
	cp -r -L $(LIBKPATH)/inc/. $(LIBCHEADERPATH)

#FIXME HACKY
#FIXME Clean this if we run make clean? surely...
libgcc: $(ARTIFACT_NEWLIB)
		$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(TOOLCHAINPATH) libgcc

publish: $(PUBLISHED_NEWLIB)

$(PUBLISHED_NEWLIB): $(ARTIFACT_NEWLIB)
	@mkdir -p $(KSYSLIBPATH)
	cp -r $(LIBCHEADERPATH)/. $(KSYSHEADERPATH)
	cp $(LIBCBINPATH)/$(TARGET)/lib/libc.a $(KSYSLIBPATH)
	cp $(LIBCBINPATH)/$(TARGET)/lib/libg.a $(KSYSLIBPATH)
	cp $(LIBCBINPATH)/$(TARGET)/lib/libm.a $(KSYSLIBPATH)

clean-all: clean-dl clean-src clean-build clean-dist

clean: clean-noredl

clean-noredl: clean-src clean-build clean-dist

clean-dl:
	rm -rf $(DLPATH)

clean-src:
	rm -rf $(SRCPATH)
	rm -f .flag.$(NAME_NEWLIB)-patched .flag.$(NAME_NEWLIB)-reconfigured

clean-build:
	rm -rf $(BUILDPATH)

clean-dist:
	rm -rf $(LIBCBINPATH)
	rm -f $(FLAG_DIST_BUILT)
