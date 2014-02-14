-include Makefile.local

.PHONY: all kernel image run debug doc clean

all: kernel cdrom.iso

kernel:
	make -C src all

cdrom.iso: build/boot/grub/grub.cfg build/kernel.elf
	@echo "creating image"
	@grub-mkrescue -o cdrom.iso build > /dev/null 2>&1

build/kernel.elf: src/kernel.elf
	@cp src/kernel.elf build/kernel.elf

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@mkdir -p build/boot/grub/
	@cp -R boot/grub/grub.cfg build/boot/grub/grub.cfg

hdd.img:
	@dd if=/dev/zero of=hdd.img count=64000 bs=1k
	@parted hdd.img -s -- mklabel msdos > /dev/null 2>&1
	@parted hdd.img -s -- mkpartfs primary fat32 1 -0 > /dev/null 2>&1

image: kernel cdrom.iso

run: image hdd.img
	@echo "running bochs"
	@$(QEMU) -smp cores=2 -boot d -cdrom cdrom.iso -drive id=disk,file=hdd.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0

doc:
	@echo "running doxygen"
	@doxygen

clean:
	make -C src clean
	rm -rf build
	rm -rf doc
	rm -f cdrom.iso
	rm -f hdd.img
