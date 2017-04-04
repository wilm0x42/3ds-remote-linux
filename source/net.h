#ifndef _FRAME_H
#define _FRAME_H

#include <3ds.h>


#define FB_SIZE (320*240*3)


extern int sock;


void net_init();
bool getFrame(int sock, u8* fb);
void sendMouseEvent(int sock, s16 x, s16 y, char click);

#endif