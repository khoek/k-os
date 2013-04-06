-include Makefile.local

.PHONY: src clean run

all: cdrom.iso

src:
	make -C src all

cdrom.iso: src
	@echo "creating image"
	@mkdir -p build/boot/grub
	@cp grub.cfg build/boot/grub/grub.cfg
	@cp src/kernel.elf build/kernel.elf
	@grub-mkrescue -o cdrom.iso build

clean:
	make -C src clean
