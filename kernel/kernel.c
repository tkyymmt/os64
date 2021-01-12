#include "fb.h"
#include "stdio.h"


struct platform_info {
	struct framebuffer fb;
	void *rsdp;
	unsigned long long nproc;
};


void start_kernel(struct platform_info *pi)
{
    fb_init(&pi->fb);

/*
    idt_init();
    kbc_init();
*/

    char ch;
    while (1) {
        ch = getchar();
        putchar(ch);
    }

    while (1);
}