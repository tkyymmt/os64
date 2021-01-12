CFLAGS = -Wall -Wextra -ffreestanding -fno-common
LDFLAGS = -Map kernel.map -s -x -T kernel.ld


kernel.bin: kernel.o stdio.o fb.o font.o kbc.o
	ld $(LDFLAGS) -o $@ $+

%.o: %.c
	gcc $(CFLAGS) -o $@ -c $<

#%.o: %.s
	#as -o $@ -c $<

clean:
	rm -rf *.o *.map kernel.bin