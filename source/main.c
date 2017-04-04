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
#include "nanojpeg.h"

int main(int argc, char **argv)
{
    gfxInitDefault();
    atexit(gfxExit);
	consoleInit(GFX_TOP, NULL);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	memset(fb, 0, FB_SIZE);
	printf("Started. Here we go!.\n");
	atexit(pauseExit);

    net_init();
	
    njInit();

    printf("Entering main loop...\n");
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		
		circlePosition cPos;
		hidCircleRead(&cPos);
	
	
        if (/* framesRunning % 20 == 0*/true)
        {
            printf("Receiving frame...\n");
            gfxFlushBuffers();
		    gfxSwapBuffers();
            getFrame(sock, fb);
            printf("Done\n");
        }
        if (kDown & KEY_START)
        {
            close(sock);
            exit(0);
        }
		if (kDown & KEY_SELECT)
		{
			(logging)? fprintf(stdout, "Disabling logging\n"):
					   fprintf(stdout, "Enabling logging\n");
			logging = !logging;
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

        // Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
	
	// Exit services
	logging = true;
	gfxExit();
	printf("Exiting...\n");
	return 0;
}
