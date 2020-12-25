#ifndef _FB_H_
#define _FB_H_


struct framebuffer {
    struct pixelformat *base;
    unsigned long long size;
    unsigned int hr;
    unsigned int vr;
};


#endif