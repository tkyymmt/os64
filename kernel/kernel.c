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

    while (1);
}