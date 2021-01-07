#include "fb.h"
#include "stdio.h"


struct platform_info {
	struct framebuffer fb;
	void *rsdp;
	unsigned long long nproc;
};

void start_kernel(struct platform_info *pi)
{
    int i = 0x10;
 
    fb_init(&pi->fb);

    printf("HELLO WORLD\n");
    printf("0x%x", i);


    while (1);
}