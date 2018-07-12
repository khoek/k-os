ROOTPATH ?= $(shell readlink -f ..)
include $(ROOTPATH)/Makefile.config

.PHONY: all clean

.DELETE_ON_ERROR:

all:
	$(MAKE) -C $(DISTIMAGEDIR) all

clean-all:
	rm -rf $(KSYSROOT)

clean: clean-image

clean-image:
	$(MAKE) -C $(DISTIMAGEDIR) clean
