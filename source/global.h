// This should be included everywhere
#ifndef _GLOBAL_H
#define _GLOBAL_H

extern bool logging;

int printLog(const char* format, ...);
#define printf printLog

void failExit(const char* msg);

void pauseExit();

#endif