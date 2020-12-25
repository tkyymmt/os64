CFLAGS = -Wall -Wextra -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -e efi_main

BOOTX64.EFI:
	x86_64-w64-mingw32-gcc $(CFLAGS) -o $@ bootx64.c

clean:
	rm -rf BOOTX64.EFI