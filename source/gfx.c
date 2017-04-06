#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "global.h"

#include "gfx/gfx_background.h"

void gfx_cpyArgbToRgb565(u8* argb, u8* rgb, int w, int h)
{
    // (srcRGB * srcA) + (dstRGB * (1-srcA))

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
        
    
        u8 r = (Or * (1 - (argb[(n*4)+3] / 0xFF))) + (argb[(n*4)  ] * (argb[(n*4)+3] / 0xFF));//r
        u8 g = (Og * (1 - (argb[(n*4)+3] / 0xFF))) + (argb[(n*4)+1] * (argb[(n*4)+3] / 0xFF));//g
        u8 b = (Ob * (1 - (argb[(n*4)+3] / 0xFF))) + (argb[(n*4)+2] * (argb[(n*4)+3] / 0xFF));//b
        
        u16 pixel = RGB8_to_565(r, g, b);
        
        memcpy(rgb+(n*2), &pixel, 2);
    }
}

void gfx_renderMenu(u8* fb, int w, int h)
{
    gfx_cpyArgbToRgb565(gfx_background, fb, w, h);
}