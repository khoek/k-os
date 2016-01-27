CCFLAGS:=-Wall -Wextra -Werror -Wno-unused-parameter -std=gnu99 -O3

SRCS+= $(shell find . -type f -name "*.c")
OBJS:= $(patsubst %.c,%,$(SRCS))

all: $(OBJS) rootramfs

%: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

rootramfs: mkrootramfs ../libc/init.c
	make -C ../libc init
	@mkdir -p fs/sbin
	@cp ../libc/init fs/sbin/init
	@./mkrootramfs -o rootramfs fs
	@echo "      mkrootramfs"

clean:
	rm -f $(OBJS)
	rm -rf fs
	rm -f rootramfs

.PHONY: all clean
