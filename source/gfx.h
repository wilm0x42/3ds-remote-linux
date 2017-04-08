#ifndef _GFX_H
#define _GFX_H

typedef struct
{
    int x;
    int y;
    int w;
    int h;
} rect_t;

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color_t;

void gfx_cpyRgbaToRgb565(u8* rgba, u8* rgb, int w, int h);
void gfx_cpyRgbaToRgb(u8* rgba, u8* rgb, int w, int h);
void gfx_fillRect565(u8* rgb, int w, int h, rect_t r, color_t c);

void gfx_renderMenu(u8* fb, int w, int h);

#endif