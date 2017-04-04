#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "net.h"

#include <3ds.h>

bool logging = true;

int printLog(const char* format, ...)
{
	if (!logging)
	{
		usleep(500);
		return 0;
	}
    va_list args;

    va_start(args, format);
    int ret = vprintf(format, args);

    va_end(args);
    return ret;
}

void failExit(const char* msg)
{
	logging = true;
    printf("Error (%d): %s\n", errno, msg);
    printf("Press start to exit.\n");
    
    while (aptMainLoop())
	{
	    gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}
	close(sock);
	exit(0);
}

void pauseExit()
{
	logging = true;
    printf("Halted (%d)\n", errno);
    printf("Press start to exit.\n");
    
    while (aptMainLoop())
	{
	    gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}
	close(sock);
	exit(0);
}
