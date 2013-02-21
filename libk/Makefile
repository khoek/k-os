-include ../Makefile.local

CFLAGS=	-Wall -Wextra -Werror -I../libk/inc -Iinc -std=gnu99 \
		-nostdlib -nostartfiles -nodefaultlibs -ffreestanding

INCS=	-Iinc

OBJS=	src/string.o src/printf.o

%.o: %.c
	@echo "      cc  $<"
	@$(GCC) $(CFLAGS) $(INCS) -c -o $@ $<

libk.a: $(OBJS)
	@echo "building libk.a"
	@ar rcs libk.a $(OBJS)

clean:
	rm -f ./*/*.o
	rm -f libk.a

.PHONY: clean
