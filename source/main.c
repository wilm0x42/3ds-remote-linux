#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <fcntl.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <3ds.h>

#include "global.h"
#include "net.h"
#include "gfx.h"
#include "ini.h"
#include "video.h"


#define TOP_WIDTH 400
#define TOP_HEIGHT 240
#define TOP_FB_SIZE (TOP_WIDTH*TOP_HEIGHT*3)

#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240
#define BOTTOM_FB_SIZE (BOTTOM_WIDTH*BOTTOM_HEIGHT*3)


int main(int argc, char **argv)
{
    gfxInit(GSP_BGR8_OES, GSP_RGB565_OES, false);
    gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB565_OES);
    atexit(gfxExit);
    
    PrintConsole consoleOut;
	consoleInit(GFX_BOTTOM, &consoleOut);
	consoleSetWindow(&consoleOut, 18, 16, 22, 14);
	
	gfxSetDoubleBuffering(GFX_TOP, false);
	u8* fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memset(fbTop, 0, TOP_FB_SIZE);
	
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	u8* fbBottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	memset(fbBottom, 0xff, BOTTOM_FB_SIZE);
	
	atexit(pauseExit);

    ini_init();//Initialize ini handler
    atexit(ini_exit);
    
    logging_verbosity = ini_getInt("loggingVerbosity");
    
    net_init();//Initialize net
    atexit(net_exit);
    
    video_init();

    printLog(1, "Entering main loop...\n");
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		
		circlePosition cPos;
		hidCircleRead(&cPos);
		
		
		gfx_renderMenu(fbBottom, BOTTOM_WIDTH, BOTTOM_HEIGHT);
	
        printLog(2, "Receiving frame...\n");
        if (getFrame(sock, fbTop, TOP_WIDTH, TOP_HEIGHT))
            printLog(2, "\x1b[32mSuccess\x1b[37m\n");
        else
            printLog(2, "\x1b[31mFailure\x1b[37m\n");
        
        if (kDown & KEY_START)
        {
            break;
        }
        
        u8 mouseBtns = 0;
        if (kDown & KEY_A) mouseBtns |= 0x01;//l
        if (kDown & KEY_Y) mouseBtns |= 0x02;//r
        if (kDown & KEY_B) mouseBtns |= 0x04;//double l
        s16 sendX, sendY;
        int divide = (kHeld & KEY_X)? 4: 8;
        sendX = (cPos.dx>15||cPos.dx<-15)?cPos.dx/divide : 0,
        sendY = (cPos.dy>15||cPos.dy<-15)?cPos.dy/divide : 0,
        printf("Sending mouse event: %hd, %hd, %hhu\n", sendX, sendY, mouseBtns);
        sendMouseEvent(sock, sendX, sendY, mouseBtns);
        printf("Done\n");
        
        if (!(kHeld & KEY_R))
        {
            u8 dpadBuf[2] = {0x05, 0};
            if (kHeld & KEY_DUP) dpadBuf[1] |= 0x01;
            if (kHeld & KEY_DDOWN) dpadBuf[1] |= 0x02;
            if (kHeld & KEY_DLEFT) dpadBuf[1] |= 0x04;
            if (kHeld & KEY_DRIGHT) dpadBuf[1] |= 0x08;
            if (dpadBuf[1])
            {
            	printf("Sending arrow keys.\n");
            	send(sock, dpadBuf, 2, 0);
            }
        }
        else
        {
            if (kDown & KEY_RIGHT && logging_verbosity < 3)
                logging_verbosity++;
            if (kDown & KEY_LEFT && logging_verbosity > 0)
                logging_verbosity--;
                
            printLog(0, "Logging verbosity: %d\n", logging_verbosity);
        }

        // Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
	
	printLog(0, "Exiting...\n");
	exit(0);
}
