CFLAGS = -Wall -Wextra -ffreestanding -fno-common
LDFLAGS = -Map kernel.map -s -x -T kernel.ld


kernel.bin: kernel.o fbcon.o fb.o font.o
	ld $(LDFLAGS) -o $@ $+

%.o: %.c
	gcc $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o *.map kernel.bin