#ifndef _FRAME_H
#define _FRAME_H

#include <3ds.h>


extern int sock;


void net_init();
void net_exit();
bool getFrame(int sock, u8* fb, int s_w, int s_h);
void sendMouseEvent(int sock, s16 x, s16 y, char click);

#endif