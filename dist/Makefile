ROOTDIR ?= $(shell readlink -f ..)
include $(ROOTDIR)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

all:
	$(MAKE) -C $(DISTIMAGEPREFIX) all

clean-all:
	rm -rf $(KSYSROOT)

clean: clean-image

clean-image:
	$(MAKE) -C $(DISTIMAGEPREFIX) clean
