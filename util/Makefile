CCFLAGS:=-Wall -Wextra -Werror -Wno-unused-parameter -std=gnu99 -O3

SRCS+= $(shell find . -type f -name "*.c")
OBJS:= $(patsubst %.c,%,$(SRCS))

all: $(OBJS)

%: %.c
	@echo "      cc  $<"
	@$(CC) $(CCFLAGS) -o $@ $<

clean:
	rm $(OBJS)

.PHONY: all clean
