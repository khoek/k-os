ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

############# TODO CHECK LIBCSTARTDIR is being output correct

DLDIR := $(TOOLCHAINPATH)/dl
SRCDIR := $(TOOLCHAINPATH)/src
PATCHDIR := $(TOOLCHAINPATH)/patch
BUILDDIR := $(TOOLCHAINPATH)/build

WGETFLAGS := -c

NAME_PERL := perl-$(PERL_VER)
NAME_AUTOCONF := autoconf-$(AUTOCONF_VER)
NAME_AUTOMAKE := automake-$(AUTOMAKE_VER)
NAME_BINUTILS := binutils-$(BINUTILS_VER)
NAME_GMP := gmp-$(GMP_VER)
NAME_MPFR := mpfr-$(MPFR_VER)
NAME_MPC := mpc-$(MPC_VER)
NAME_GCC := gcc-$(GCC_VER)

AR_PERL := $(NAME_PERL).tar.gz
AR_AUTOCONF := $(NAME_AUTOCONF).tar.gz
AR_AUTOMAKE := $(NAME_AUTOMAKE).tar.gz
AR_BINUTILS := $(NAME_BINUTILS).tar.gz
AR_GMP := $(NAME_GMP).tar.xz
AR_MPFR := $(NAME_MPFR).tar.gz
AR_MPC := $(NAME_MPC).tar.gz
AR_GCC := $(NAME_GCC).tar.gz

URL_PERL := http://www.cpan.org/src/5.0/$(AR_PERL)
URL_AUTOCONF := https://ftp.gnu.org/gnu/autoconf/$(AR_AUTOCONF)
URL_AUTOMAKE := https://ftp.gnu.org/gnu/automake/$(AR_AUTOMAKE)
URL_BINUTILS := http://ftp.gnu.org/gnu/binutils/$(AR_BINUTILS)
URL_GMP := http://ftp.gnu.org/gnu/gmp/$(AR_GMP)
URL_MPFR := http://ftp.gnu.org/gnu/mpfr/$(AR_MPFR)
URL_MPC := https://ftp.gnu.org/gnu/mpc/$(AR_MPC)
URL_GCC := http://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VER)/$(AR_GCC)

ARTIFACT_PERL := $(TOOLCHAINBINPATH)/bin/perl
ARTIFACT_AUTOCONF := $(TOOLCHAINBINPATH)/bin/autoconf
ARTIFACT_AUTOMAKE := $(TOOLCHAINBINPATH)/bin/automake
ARTIFACT_BINUTILS := $(TOOLCHAINBINPATH)/bin/$(TARGET)-ld
ARTIFACT_GMP := $(TOOLCHAINBINPATH)/lib/libgmp.a
ARTIFACT_MPFR := $(TOOLCHAINBINPATH)/lib/libmpfr.a
ARTIFACT_MPC := $(TOOLCHAINBINPATH)/lib/libmpc.a
ARTIFACT_GCC := $(TOOLCHAINBINPATH)/bin/$(TARGET)-gcc
ARTIFACT_LIBGCC := $(TOOLCHAINBINPATH)/lib/gcc/$(TARGET)/$(GCC_VER)/libgcc.a

PATCH_AUTOCONF := $(PATCHDIR)/$(NAME_AUTOCONF).patch
PATCH_BINUTILS := $(PATCHDIR)/$(NAME_BINUTILS).patch
PATCH_GCC := $(PATCHDIR)/$(NAME_GCC).patch

export PERL := $(ARTIFACT_PERL)
export AUTOCONF := $(ARTIFACT_AUTOCONF)
export AUTOMAKE := $(ARTIFACT_AUTOMAKE)
export ACLOCAL := $(TOOLCHAINBINPATH)/bin/aclocal
export MAKEINFO := true

BISON := $(shell command -v bison 2> /dev/null)
FLEX := $(shell command -v flex 2> /dev/null)

ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif

FLAG_DIST_BUILT := .flag.dist-built

.PHONY: all required tools libgcc dl check check-binutils check-gmp check-mpfr check-mpc clean-full clean clean-dl clean-src clean-build clean-noredl

.DELETE_ON_ERROR:

all: tools check
	touch $(FLAG_DIST_BUILT)

$(FLAG_DIST_BUILT):
	$(MAKE) all

required: $(FLAG_DIST_BUILT)

tools: $(ARTIFACT_AUTOCONF) $(ARTIFACT_AUTOMAKE) $(ARTIFACT_BINUTILS) $(ARTIFACT_GCC)

libgcc: $(ARTIFACT_LIBGCC)

dl: $(DLDIR)/$(AR_PERL) $(DLDIR)/$(AR_AUTOMAKE) $(DLDIR)/$(AR_AUTOCONF) $(DLDIR)/$(AR_BINUTILS) $(DLDIR)/$(AR_GMP) $(DLDIR)/$(AR_MPFR) $(DLDIR)/$(AR_MPC) $(DLDIR)/$(AR_GCC)

$(DLDIR)/$(AR_PERL):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_PERL)

$(DLDIR)/$(AR_AUTOCONF):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_AUTOCONF)

$(DLDIR)/$(AR_AUTOMAKE):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_AUTOMAKE)

$(DLDIR)/$(AR_BINUTILS):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_BINUTILS)

$(DLDIR)/$(AR_GMP):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_GMP)

$(DLDIR)/$(AR_MPFR):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_MPFR)

$(DLDIR)/$(AR_MPC):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_MPC)

$(DLDIR)/$(AR_GCC):
	$(WGET) -P $(DLDIR) $(WGETFLAGS) $(URL_GCC)

$(SRCDIR)/$(NAME_PERL)/Configure: $(DLDIR)/$(AR_PERL)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_PERL)
	touch $@

$(SRCDIR)/$(NAME_AUTOCONF)/configure: $(DLDIR)/$(AR_AUTOCONF)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_AUTOCONF)
	touch $@

$(SRCDIR)/$(NAME_AUTOMAKE)/configure: $(DLDIR)/$(AR_AUTOMAKE)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_AUTOMAKE)
	touch $@

$(SRCDIR)/$(NAME_BINUTILS)/configure: $(DLDIR)/$(AR_BINUTILS)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_BINUTILS)
	touch $@

$(SRCDIR)/$(NAME_GMP)/configure: $(DLDIR)/$(AR_GMP)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_GMP)
	touch $@

$(SRCDIR)/$(NAME_MPFR)/configure: $(DLDIR)/$(AR_MPFR)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_MPFR)
	touch $@

$(SRCDIR)/$(NAME_MPC)/configure: $(DLDIR)/$(AR_MPC)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_MPC)
	touch $@

$(SRCDIR)/$(NAME_GCC)/configure: $(DLDIR)/$(AR_GCC)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_GCC)
	touch $@

.flag.$(NAME_BINUTILS)-patched: $(SRCDIR)/$(NAME_BINUTILS)/configure
	patch -p0 < $(PATCH_BINUTILS)
	sed -i "s_KOSLIBDIRS_/$(KSYSLIBDIR)_g" $(SRCDIR)/$(NAME_BINUTILS)/ld/configure.tgt
	touch $@

.flag.$(NAME_BINUTILS)-reconfigured: .flag.$(NAME_BINUTILS)-patched $(ARTIFACT_AUTOMAKE)
	(cd $(SRCDIR)/$(NAME_BINUTILS)/ld; $(ARTIFACT_AUTOMAKE))
	touch $@

.flag.$(NAME_GCC)-patched: $(SRCDIR)/$(NAME_GCC)/configure
	patch -p0 < $(PATCH_GCC)
	sed -i "s_KOSSYSHEADDIR_/$(KSYSHEADERDIR)_g" $(SRCDIR)/$(NAME_GCC)/gcc/config.gcc
	rm -f $(SRCDIR)/$(NAME_GCC)/gcc/config/kos-autogen.h
	echo "#undef STANDARD_STARTFILE_PREFIX\n#define STANDARD_STARTFILE_PREFIX \"$(KSYSSTARTPATH)\"\n" > $(SRCDIR)/$(NAME_GCC)/gcc/config/kos-autogen.h
	touch $@

.flag.$(NAME_GCC)-reconfigured: .flag.$(NAME_GCC)-patched $(ARTIFACT_AUTOCONF)
	(cd $(SRCDIR)/$(NAME_GCC)/libstdc++-v3; $(ARTIFACT_AUTOCONF))
	touch $@

#Note: Perl must be compiled in the source dir
$(SRCDIR)/$(NAME_PERL)/Makefile: $(SRCDIR)/$(NAME_PERL)/Configure
	mkdir -p $(TOOLCHAINBINPATH)
	cd $(SRCDIR)/$(NAME_PERL) && sh $< -des -Dprefix=$(TOOLCHAINBINPATH) -Dotherlibdirs=$(TOOLCHAINBINPATH)/lib/perl5
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_AUTOCONF)/Makefile: $(SRCDIR)/$(NAME_AUTOCONF)/configure $(ARTIFACT_PERL)
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_AUTOCONF)
	cd $(BUILDDIR)/$(NAME_AUTOCONF) && sh $< -v --prefix=$(TOOLCHAINBINPATH)
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_AUTOMAKE)/Makefile: $(SRCDIR)/$(NAME_AUTOMAKE)/configure $(ARTIFACT_PERL) $(ARTIFACT_AUTOCONF)
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_AUTOMAKE)
	cd $(BUILDDIR)/$(NAME_AUTOMAKE) && sh $< -v --prefix=$(TOOLCHAINBINPATH)
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_BINUTILS)/Makefile: .flag.$(NAME_BINUTILS)-reconfigured
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_BINUTILS)
	cd $(BUILDDIR)/$(NAME_BINUTILS) && sh $(SRCDIR)/$(NAME_BINUTILS)/configure $(TOOLCHAIN_CONFFLAGS) --target=$(TARGET) --prefix=$(TOOLCHAINBINPATH) --with-sysroot=$(KSYSROOT)
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_GMP)/Makefile: $(SRCDIR)/$(NAME_GMP)/configure
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_GMP)
	cd $(BUILDDIR)/$(NAME_GMP) && sh $< $(TOOLCHAIN_CONFFLAGS) --prefix=$(TOOLCHAINBINPATH) --disable-shared
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_MPFR)/Makefile: $(SRCDIR)/$(NAME_MPFR)/configure $(ARTIFACT_GMP)
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_MPFR)
	cd $(BUILDDIR)/$(NAME_MPFR) && sh $< $(TOOLCHAIN_CONFFLAGS) --prefix=$(TOOLCHAINBINPATH) --with-gmp=$(TOOLCHAINBINPATH) --disable-shared
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_MPC)/Makefile: $(SRCDIR)/$(NAME_MPC)/configure $(ARTIFACT_GMP) $(ARTIFACT_MPFR)
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_MPC)
	cd $(BUILDDIR)/$(NAME_MPC) && sh $< $(TOOLCHAIN_CONFFLAGS) --target=$(TARGET) --prefix=$(TOOLCHAINBINPATH) --with-gmp=$(TOOLCHAINBINPATH) --with-mpfr=$(TOOLCHAINBINPATH) --disable-shared
	cd $(TOOLCHAINPATH)

$(BUILDDIR)/$(NAME_GCC)/Makefile: .flag.$(NAME_GCC)-reconfigured $(ARTIFACT_BINUTILS) $(ARTIFACT_GMP) $(ARTIFACT_MPFR) $(ARTIFACT_MPC)
	mkdir -p $(KSYSHEADERPATH)
	mkdir -p $(TOOLCHAINBINPATH)
	mkdir -p $(BUILDDIR)/$(NAME_GCC)
	cd $(BUILDDIR)/$(NAME_GCC) && sh $(SRCDIR)/$(NAME_GCC)/configure $(TOOLCHAIN_CONFFLAGS) --target=$(TARGET) --prefix=$(TOOLCHAINBINPATH) --enable-languages=c --without-headers --disable-nls --with-gmp=$(TOOLCHAINBINPATH) --with-mpfr=$(TOOLCHAINBINPATH) --with-mpc=$(TOOLCHAINBINPATH) --with-sysroot=$(KSYSROOT)
	cd $(TOOLCHAINPATH)

$(ARTIFACT_PERL): $(SRCDIR)/$(NAME_PERL)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_AUTOCONF): $(BUILDDIR)/$(NAME_AUTOCONF)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_AUTOMAKE): $(BUILDDIR)/$(NAME_AUTOMAKE)/Makefile
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install
	ln -s $(TOOLCHAINBINPATH)/share/aclocal-* $(TOOLCHAINBINPATH)/share/aclocal

$(ARTIFACT_BINUTILS): $(BUILDDIR)/$(NAME_BINUTILS)/Makefile
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<)
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install

$(ARTIFACT_GMP): $(BUILDDIR)/$(NAME_GMP)/Makefile
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<)
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install

$(ARTIFACT_MPFR): $(BUILDDIR)/$(NAME_MPFR)/Makefile
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<)
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install

$(ARTIFACT_MPC): $(BUILDDIR)/$(NAME_MPC)/Makefile
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<)
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install

$(ARTIFACT_GCC): $(BUILDDIR)/$(NAME_GCC)/Makefile
	mkdir -p $(KSYSHEADERPATH)
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) all-gcc
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install-gcc

$(ARTIFACT_LIBGCC): $(BUILDDIR)/$(NAME_GCC)/Makefile
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) all-target-libgcc
	$(MAKE) $(TOOLCHAIN_MAKEFLAGS) -C $(shell dirname $<) install-target-libgcc

check: check-binutils check-gmp check-mpfr check-mpc

.flag.checked-$(NAME_BINUTILS): $(ARTIFACT_BINUTILS)
	$(MAKE) -C $(BUILDDIR)/$(NAME_BINUTILS) check
	touch $@

check-binutils: .flag.checked-$(NAME_BINUTILS)

.flag.checked-$(NAME_GMP): $(ARTIFACT_GMP)
	$(MAKE) -C $(BUILDDIR)/$(NAME_GMP) check
	touch $@

check-gmp: .flag.checked-$(NAME_GMP)

.flag.checked-$(NAME_MPFR): $(ARTIFACT_MPFR)
	$(MAKE) -C $(BUILDDIR)/$(NAME_MPFR) check
	touch $@

check-mpfr: .flag.checked-$(NAME_MPFR)

.flag.checked-$(NAME_MPC): $(ARTIFACT_MPC)
	$(MAKE) -C $(BUILDDIR)/$(NAME_MPC) check
	touch $@

check-mpc: .flag.checked-$(NAME_MPC)

clean-all: clean-dl clean-noredl

clean: clean-noredl

clean-noredl: clean-src clean-build clean-dist

clean-dl:
	rm -rf $(DLDIR)

clean-src:
	rm -rf $(SRCDIR)
	rm -f .flag.$(NAME_BINUTILS)-patched .flag.$(NAME_BINUTILS)-reconfigured
	rm -f .flag.$(NAME_GCC)-patched .flag.$(NAME_GCC)-reconfigured

clean-build:
	rm -rf $(BUILDDIR)

clean-dist:
	rm -rf $(TOOLCHAINBINPATH)
	rm -f $(FLAG_DIST_BUILT)
