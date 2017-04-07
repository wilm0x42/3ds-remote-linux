#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "global.h"
#include "video.h"
#include "gfx.h"

#include "nanojpeg.h"
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#include "lodepng.h"


void video_init()
{
    njInit();//Initialize NanoJpeg
}

bool video_decodeJpg(char* fileBuf, int fileSize, u8* fb, int w, int h)
{
    if (njDecode(fileBuf, fileSize))
	{
        printLog(2, "Error decoding JPG\n");
        return true;
    }
        
    printf("Copying to fb...\n");
    if (njGetWidth() == h && njGetHeight() == w && njIsColor())
    {
    	memcpy(fb, njGetImage(), njGetImageSize());
    }
    else
    {
        printLog(2, "Error: Jpg is wrong size or format (%dx%d)\n",
		        njGetWidth(),
		        njGetHeight());
	    return true;
    }
    
    njDone(); 
    return false;
}

bool video_decodePng(char* fileBuf, int fileSize, u8* fb, int w, int h)
{
    unsigned char* decodeBuf;
    unsigned int decodeW, decodeH;
    
    if (lodepng_decode32(&decodeBuf, &decodeW, &decodeH,
                         (unsigned char*)fileBuf, fileSize))
    {
        printLog(2, "Error decoding PNG\n");
        return true;
    }
    
    if (decodeW == w && decodeH == h)
    {
        gfx_cpyRgbaToRgb(decodeBuf, fb, w, h);
    }
    else
    {
        printLog(2, "Error: Png is wrong size (%dx%d)\n", decodeW, decodeH);
        return true;
    }
    
    free(decodeBuf);
    return false;
}