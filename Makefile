include Makefile.local

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
	make -C ../bochs
	@echo "running bochs"
	@echo "c" | $(BOCHS) -q

build/boot/grub/grub.cfg: boot/grub/grub.cfg
	@cp -R boot build

build/kernel.elf: libk/libk.a kernel/kernel.a
	@echo "linking kernel"
	@$(LD) -T linker.ld -o build/kernel.elf kernel/kernel.a -Llibk -lk

cdrom.iso: build/boot/grub/grub.cfg build/kernel.elf
	@echo "creating image"
	grub-mkrescue -o cdrom.iso build

