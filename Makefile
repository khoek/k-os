-include Makefile.local

.PHONY: all clean run

all: cdrom.iso

src/kernel.elf:
	make -C src all
	
cdrom.iso: src/kernel.elf build/boot/grub/grub.cfg
	@echo "creating image"
	@cp src/kernel.elf build/kernel.elf
	@grub-mkrescue -o cdrom.iso build

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@cp -R boot build

hdd.img:
	@dd if=/dev/zero of=hdd.img count=512

run: cdrom.iso hdd.img
	@echo "running qemu"
	@$(QEMU) -cdrom cdrom.iso -drive id=disk,file=hdd.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0

clean:
	make -C src clean
