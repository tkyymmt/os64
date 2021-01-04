#include "fb.h"
#include "fbcon.h"
#include "font.h"


void start_kernel(struct framebuffer *fb)
{
    fb_init(fb);
    set_fg(255, 255, 255);
    set_bg(0, 0, 0);
    clear_screen();

    puts("HELLO WORLD!");

    while (1);
}