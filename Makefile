-include Makefile.local

.PHONY: all kernel image run clean

all: kernel cdrom.iso

kernel:
	make -C src all
	
cdrom.iso: src/kernel.elf build/boot/grub/grub.cfg
	@echo "creating image"
	@cp src/kernel.elf build/kernel.elf
	@grub-mkrescue -o cdrom.iso build

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@cp -R boot build

hdd.img:
	@dd if=/dev/zero of=hdd.img count=512

image: kernel cdrom.iso

run: image hdd.img
	@echo "running qemu"
	@$(QEMU) -cdrom cdrom.iso -hda hdd.img

clean:
	make -C src clean
	rm -f cdrom.iso
	rm -f kernel.sys

