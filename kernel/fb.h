#ifndef _FB_H_
#define _FB_H_


struct framebuffer {
    struct pixelformat *base;
    unsigned long long size;
    unsigned int hr;
    unsigned int vr;
};

extern struct framebuffer fb;

void fb_init(struct framebuffer *_fb);
void draw_px_fg(unsigned int x, unsigned int y);
void clear_screen(void);



#endif