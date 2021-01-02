run: b k
	mkdir -p fs/EFI/BOOT
	mv bootloader/BOOTX64.EFI fs/EFI/BOOT
	mv kernel/kernel.bin fs
	qemu-system-x86_64 -bios OVMF.fd -drive format=raw,file=fat:rw:fs -m 4G -monitor stdio

b:
	make -C bootloader

k:
	make -C kernel

clean:
	make $@ -C bootloader
	make $@ -C kernel
	rm -rf fs

.PHONY: run b k clean
