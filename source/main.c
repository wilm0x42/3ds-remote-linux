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
#include "ini.h"

#include "nanojpeg.h"


#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#define FB_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT*3)


int main(int argc, char **argv)
{
    gfxInitDefault();
    atexit(gfxExit);
	consoleInit(GFX_BOTTOM, NULL);
	gfxSetDoubleBuffering(GFX_TOP, false);
	u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memset(fb, 0, FB_SIZE);
	atexit(pauseExit);

    ini_init();
    atexit(ini_exit());
    
    net_init();
	
    njInit();

    printLog(0, "Entering main loop...\n");
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		
		circlePosition cPos;
		hidCircleRead(&cPos);
	
	
        if (/* framesRunning % 20 == 0*/true)
        {
            printLog(1, "Receiving frame...\n");
            if (getFrame(sock, fb, SCREEN_WIDTH, SCREEN_HEIGHT))
                printLog(1, "\x1b[32mSuccess\x1b[37m\n");
            else
                printLog(1, "\x1b[31mFailure\x1b[37m\n");
        }
        if (kDown & KEY_START)
        {
            close(sock);
            exit(0);
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
	
	// Exit services
	gfxExit();
	printLog(0, "Exiting...\n");
	return 0;
}
