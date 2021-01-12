#include "fb.h"
#include "stdio.h"


struct platform_info {
	struct framebuffer fb;
	void *rsdp;
	unsigned long long nproc;
};

unsigned short *vram = 0xB8000;


void start_kernel(struct platform_info *pi)
{
    fb_init(&pi->fb);

    while(1) {
        char ch = getchar();
        if (('a' <= ch) && (ch <= 'z'))
            ch = ch - 'a' + 'A';
        putchar(ch);
    }

    while (1);
}