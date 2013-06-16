-include Makefile.local

.PHONY: all kernel image run debug doc clean

all: kernel cdrom.iso

kernel:
	make -C src all

cdrom.iso: src/kernel.elf build/boot/grub/grub.cfg
	@echo "creating image"
	@cp src/kernel.elf build/kernel.elf
	@grub-mkrescue -o cdrom.iso build

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@mkdir -p build/boot/grub/
	@cp -R boot/grub/grub.cfg build/boot/grub/grub.cfg

hdd.img:
	@dd if=/dev/zero of=hdd.img count=2048

image: kernel cdrom.iso

run: image hdd.img
	@echo "running qemu"
	@$(QEMU) -boot d -cdrom cdrom.iso -hda hdd.img

debug: image hdd.img
	@echo "running bochs"
	@$(BOCHS) -q
	
doc:
	@echo "running doxygen"
	@doxygen

clean:
	make -C src clean
	rm -rf build
	rm -f cdrom.iso
	rm -f hdd.img
