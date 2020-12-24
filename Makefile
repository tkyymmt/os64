all: fs/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -nographic -bios OVMF.fd -drive format=raw,file=fat:rw:fs

fs/EFI/BOOT/BOOTX64.EFI:
	mkdir -p fs/EFI/BOOT
	x86_64-w64-mingw32-gcc -Wall -Wextra -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -e efi_main -o $@ bootx64.c

clean:
	rm -rf fs