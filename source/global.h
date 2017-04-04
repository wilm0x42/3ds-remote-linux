// This should be included everywhere
#ifndef _GLOBAL_H
#define _GLOBAL_H

extern int logging_verbosity;

int printLog(int verbosity, const char* format, ...);
int printLogSimple(const char* format, ...);
#define printf printLogSimple

void failExit(const char* msg);

void pauseExit();

#endif