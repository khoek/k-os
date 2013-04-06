-include Makefile.local

.PHONY: src clean run

all: cdrom.iso

src:
	make -C src all

cdrom.iso: src
	@echo "creating image"
	@cp -R grub.cfg build/boot/grub/
	@cp -R kernel.elf src/kernel.elf
	@grub-mkrescue -o cdrom.iso build

clean:
	make -C src clean