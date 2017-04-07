#ifndef _VIDEO_H
#define _VIDEO_H

#define FTYPE_JPG 0
#define FTYPE_PNG 1

#define MODE_DYNAMIC 0
#define MODE_STATIC 1


extern unsigned char video_quality;
extern unsigned char video_mode;


void video_init();

bool video_decodeJpg(char* fileBuf, int fileSize, u8* fb, int w, int h);
bool video_decodePng(char* fileBuf, int fileSize, u8* fb, int w, int h);

#endif