#ifndef _GFX_H
#define _GFX_H

void gfx_cpyArgbToRgb565(u8* rgba, u8* rgb, int w, int h);

void gfx_renderMenu(u8* fb, int w, int h);

#endif