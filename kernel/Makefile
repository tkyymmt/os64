CFLAGS = -Wall -Wextra -ffreestanding -fno-common
LDFLAGS = -Map kernel.map -s -x -T kernel.ld

OBJS := *.o

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -o $@ $+

%.o: %.c
	gcc $(CFLAGS) -o $@ -c $<

%.o: %.s
	as -o $@ -c $<

clean:
	rm -rf $(OBJS) *.map kernel.bin