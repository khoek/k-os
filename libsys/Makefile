-include ../Makefile.local

ASFLAGS=--32 -g

CCFLAGS=-Wall -g -Wextra -Werror -Wno-unused-parameter -m32 -I../inc -std=gnu99 \
        -nostdlib -nostartfiles -nodefaultlibs -ffreestanding -c

OBJS=	src/crt.o src/syscall.o

all: libsys.a

.s.o:
	@echo "      as  $<"
	@$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

libsys.a: $(OBJS)
	@echo "      ar  libsys.a"
	@rm -f libsys.a
	@$(AR) rcs libsys.a $(OBJS)

clean:
	find . -name \*.o -type f -delete
	rm -f libsys.a

.PHONY: all clean
