-include Makefile.local

all: cdrom.iso

libk/libk.a:
	@make -C libk libk.a

kernel/kernel.a:
	@make -C kernel kernel.a

.PHONY: libc/libc.a kernel/kernel.a clean run

clean:
	make -C libk clean
	make -C kernel clean
	rm -f kernel.elf
	rm -f cdrom.iso
	rm -rf build/*

run: cdrom.iso
	@echo "running qemu"
	@qemu-system-x86_64 -cdrom cdrom.iso -drive id=disk,file=hdd.img,if=none -device ahci,id=ahci -device ide-drive,drive=disk,bus=ahci.0

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@cp -R boot build

build/kernel.elf: libk/libk.a kernel/kernel.a
	@make -C kernel ../build/kernel.elf

cdrom.iso: build/boot/grub/grub.cfg build/kernel.elf
	@echo "creating image"
	grub-mkrescue -o cdrom.iso build
