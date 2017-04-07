#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <3ds.h>

int logging_verbosity = 1;

int printLog(int verbosity, const char* format, ...)
{
	if (verbosity > logging_verbosity)
	{
		//usleep(500);
		return 0;
	}
    va_list args;

    va_start(args, format);
    int ret = vprintf(format, args);

    va_end(args);
    return ret;
}

int printLogSimple(const char* format, ...)
{
	if (logging_verbosity < 3)
		return 0;

	va_list args;

    va_start(args, format);
    int ret = vprintf(format, args);

    va_end(args);
    return ret;
}

void failExit(const char* msg)
{
    printLog(0, "Error (%d): %s\n", errno, msg);
    printLog(0, "Press start to exit.\n");
    
    while (aptMainLoop())
	{
	    gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}
	exit(0);
}

void pauseExit()
{
    printLog(0, "Halted (%d)\n", errno);
    printLog(0, "Press start to exit.\n");
    
    while (aptMainLoop())
	{
	    gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}
	exit(0);
}
