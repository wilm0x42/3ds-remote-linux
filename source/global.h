// This should be included everywhere
#ifndef _GLOBAL_H
#define _GLOBAL_H


#define pointInRect(x, y, rx, ry, rw, rh)\
    (x <= rx+rw && x >= rx && y <= ry+rh && y >= ry)


extern int logging_verbosity;

int printLog(int verbosity, const char* format, ...);
int printLogSimple(const char* format, ...);
#define printf printLogSimple

void failExit(const char* msg);

void pauseExit();

#endif