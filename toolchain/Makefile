ROOTDIR ?= $(shell readlink -f .)/..
include $(ROOTDIR)/Makefile.config

DLDIR := $(TOOLCHAINDIR)/dl
SRCDIR := $(TOOLCHAINDIR)/src
BUILDDIR := $(TOOLCHAINBINDIR)

WGETFLAGS := -c

ARTIFACT_BINUTILS := $(BUILDDIR)/bin/$(TARGET)-ld
ARTIFACT_GMP := $(BUILDDIR)/lib/libgmp.a
ARTIFACT_MPFR := $(BUILDDIR)/lib/libmpfr.a
ARTIFACT_MPC := $(BUILDDIR)/lib/libmpc.a
ARTIFACT_GCC := $(BUILDDIR)/bin/$(TARGET)-gcc

NAME_BINUTILS := binutils-$(BINUTILS_VER)
NAME_GMP := gmp-$(GMP_VER)
NAME_MPFR := mpfr-$(MPFR_VER)
NAME_MPC := mpc-$(MPC_VER)
NAME_GCC := gcc-$(GCC_VER)

AR_BINUTILS := $(NAME_BINUTILS).tar.gz
AR_GMP := $(NAME_GMP).tar.xz
AR_MPFR := $(NAME_MPFR).tar.gz
AR_MPC := $(NAME_MPC).tar.gz
AR_GCC := $(NAME_GCC).tar.gz

URL_BINUTILS := http://ftp.gnu.org/gnu/binutils/$(AR_BINUTILS)
URL_GMP := http://ftp.gnu.org/gnu/gmp/$(AR_GMP)
URL_MPFR := http://ftp.gnu.org/gnu/mpfr/$(AR_MPFR)
URL_MPC := http://www.multiprecision.org/mpc/download/$(AR_MPC)
URL_GCC := http://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VER)/$(AR_GCC)

BISON := $(shell command -v bison 2> /dev/null)
FLEX := $(shell command -v flex 2> /dev/null)
AUTOMAKE := $(shell command -v automake 2> /dev/null)

.PHONY: all clean clean-dl clean-src clean-build clean-noredl check check-binutils check-gmp check-mpfr check-mpc

all: $(ARTIFACT_GCC) check

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

$(SRCDIR)/$(NAME_BINUTILS)/configure: $(DLDIR)/$(AR_BINUTILS)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_BINUTILS)
	touch $@

$(SRCDIR)/$(NAME_GMP)/configure: $(DLDIR)/$(AR_GMP)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_GMP)
	touch $@

$(SRCDIR)/$(NAME_MPFR)/configure: $(ARTIFACT_GMP) $(DLDIR)/$(AR_MPFR)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_MPFR)
	touch $@

$(SRCDIR)/$(NAME_MPC)/configure: $(ARTIFACT_GMP) $(ARTIFACT_MPFR) $(DLDIR)/$(AR_MPC)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_MPC)
	touch $@

$(SRCDIR)/$(NAME_GCC)/configure: $(ARTIFACT_BINUTILS) $(ARTIFACT_GMP) $(ARTIFACT_MPFR) $(ARTIFACT_MPC) $(DLDIR)/$(AR_GCC)
	mkdir -p $(SRCDIR)
	$(TAR) -C $(SRCDIR) -xf $(DLDIR)/$(AR_GCC)
	touch $@

$(SRCDIR)/$(NAME_BINUTILS)/Makefile: $(SRCDIR)/$(NAME_BINUTILS)/configure
	mkdir -p $(BUILDDIR)
	cd $(SRCDIR)/$(NAME_BINUTILS) && sh $< --target=$(TARGET) --prefix=$(BUILDDIR)
	cd $(TOOLCHAINDIR)

$(SRCDIR)/$(NAME_GMP)/Makefile: $(SRCDIR)/$(NAME_GMP)/configure
	mkdir -p $(BUILDDIR)
	cd $(SRCDIR)/$(NAME_GMP) && sh $< --prefix=$(BUILDDIR) --disable-shared
	cd $(TOOLCHAINDIR)

$(SRCDIR)/$(NAME_MPFR)/Makefile: $(SRCDIR)/$(NAME_MPFR)/configure
	mkdir -p $(BUILDDIR)
	cd $(SRCDIR)/$(NAME_MPFR) && sh $< --prefix=$(BUILDDIR) --with-gmp=$(BUILDDIR) --disable-shared
	cd $(TOOLCHAINDIR)

$(SRCDIR)/$(NAME_MPC)/Makefile: $(SRCDIR)/$(NAME_MPC)/configure
	mkdir -p $(BUILDDIR)
	cd $(SRCDIR)/$(NAME_MPC) && sh $< --target=$(TARGET) --prefix=$(BUILDDIR) --with-gmp=$(BUILDDIR) --with-mpfr=$(BUILDDIR) --disable-shared
	cd $(TOOLCHAINDIR)

$(SRCDIR)/$(NAME_GCC)/Makefile: $(SRCDIR)/$(NAME_GCC)/configure
	mkdir -p $(BUILDDIR)
	cd $(SRCDIR)/$(NAME_GCC) && sh $< --target=$(TARGET) --prefix=$(BUILDDIR) --enable-languages=c --without-headers --disable-nls --with-gmp=$(BUILDDIR) --with-mpfr=$(BUILDDIR) --with-mpc=$(BUILDDIR)
	cd $(TOOLCHAINDIR)

$(ARTIFACT_BINUTILS): $(SRCDIR)/$(NAME_BINUTILS)/Makefile
ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif
ifndef AUTOMAKE
    $(error "'automake' not available, please install it")
endif
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_GMP): $(SRCDIR)/$(NAME_GMP)/Makefile
ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif
ifndef AUTOMAKE
    $(error "'automake' not available, please install it")
endif
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_MPFR): $(SRCDIR)/$(NAME_MPFR)/Makefile
ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif
ifndef AUTOMAKE
    $(error "'automake' not available, please install it")
endif
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_MPC): $(SRCDIR)/$(NAME_MPC)/Makefile
ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif
ifndef AUTOMAKE
    $(error "'automake' not available, please install it")
endif
	$(MAKE) -C $(shell dirname $<)
	$(MAKE) -C $(shell dirname $<) install

$(ARTIFACT_GCC): $(SRCDIR)/$(NAME_GCC)/Makefile
ifndef BISON
    $(error "'bison' not available, please install it")
endif
ifndef FLEX
    $(error "'flex' not available, please install it")
endif
ifndef AUTOMAKE
    $(error "'automake' not available, please install it")
endif
	$(MAKE) -C $(shell dirname $<) all-gcc
	$(MAKE) -C $(shell dirname $<) install-gcc

check: check-binutils check-gmp check-mpfr check-mpc

check-binutils: $(ARTIFACT_BINUTILS)
	$(MAKE) -C $(SRCDIR)/$(NAME_BINUTILS) check

check-gmp: $(ARTIFACT_GMP)
	$(MAKE) -C $(SRCDIR)/$(NAME_GMP) check

check-mpfr: $(ARTIFACT_MPFR)
	$(MAKE) -C $(SRCDIR)/$(NAME_MPFR) check

check-mpc: $(ARTIFACT_MPC)
	$(MAKE) -C $(SRCDIR)/$(NAME_MPC) check

clean: clean-dl clean-src clean-build

clean-dl:
	@rm -rf $(DLDIR)

clean-src:
	@rm -rf $(SRCDIR)

clean-build:
	@rm -rf $(BUILDDIR)

clean-noredl: clean-src clean-build
	@rm -rf $(SRCDIR)
