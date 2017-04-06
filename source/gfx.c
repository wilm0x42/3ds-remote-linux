#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "global.h"

#include "gfx/gfx_background.h"

void gfx_cpyRgbaToRgb565(u8* rgba, u8* rgb, int w, int h)
{
    for (int n = 0; n < (w * h); n++)
    {
        u16 Opixel;
        memcpy(&Opixel, rgb+(n*2), 2);
        
        u8 Or = (Opixel & 0xF800) >> 11;
        u8 Og = (Opixel & 0x07E0) >> 5;
        u8 Ob = Opixel & 0x001F;

        Or = (Or * 255) / 31;
        Og = (Og * 255) / 63;
        Ob = (Ob * 255) / 31;
        
        // (srcRGB * srcA) + (dstRGB * (1-srcA))
        u8 r = (Or * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)  ] * (rgba[(n*4)+3] / 0xFF));//r
        u8 g = (Og * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)+1] * (rgba[(n*4)+3] / 0xFF));//g
        u8 b = (Ob * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)+2] * (rgba[(n*4)+3] / 0xFF));//b
        
        u16 pixel = RGB8_to_565(r, g, b);
        
        memcpy(rgb+(n*2), &pixel, 2);
    }
}

void gfx_cpyRgbaToRgb(u8* rgba, u8* rgb, int w, int h)
{
    for (int n = 0; n < (w*h); n++)
    {
        rgb[n*3  ] = (rgb[n*3  ] * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)  ] * (rgba[(n*4)+3] / 0xFF));//r
        rgb[n*3+1] = (rgb[n*3+1] * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)+1] * (rgba[(n*4)+3] / 0xFF));//g
        rgb[n*3+2] = (rgb[n*3+2] * (1 - (rgba[(n*4)+3] / 0xFF))) + (rgba[(n*4)+2] * (rgba[(n*4)+3] / 0xFF));//b
    }
}

void gfx_renderMenu(u8* fb, int w, int h)
{
    gfx_cpyRgbaToRgb565(gfx_background, fb, w, h);
}