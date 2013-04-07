.PHONY: all clean run

all: cdrom.iso

src/kernel.elf:
	make -C src all
	
cdrom.iso: src/kernel.elf
	@echo "creating image"
	@mkdir build
	@cp boot build
	@cp src/kernel.elf build/kernel.elf
	@grub-mkrescue -o cdrom.iso build
	
run: cdrom.iso
	@echo "running qemu"
	@qemu-system-x86_64 -cdrom cdrom.iso -drive id=disk,file=hdd.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0

clean:
	make -C src clean
