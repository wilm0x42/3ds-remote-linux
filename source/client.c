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

//#include "lodepng.h"
#include "nanojpeg.h"

#define FB_SIZE (320*240*3)

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;
int sock;

static bool logging = true;
#define printf printLog

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
	return;
}

void socShutdown()
{
	printf("waiting for socExit...\n");
	socExit();
}

bool allChunks(bool* chunks, uint16_t chunkCount, uint16_t* nextChunk)
{
    for (uint16_t i = 0; i < chunkCount; i++)
    {
        if (chunks[i] == false)
        {
            if (nextChunk) *nextChunk = i;
            return false;
        }
    }
    return true;
}

bool requestChunk(int sock, char* buf, uint16_t chunkSize, uint16_t whichChunk, uint16_t fileId)
{
    char rqBuf[1+4];
    uint16_t sendWhichChunk = htons(whichChunk),
             sendFileId = htons(fileId);
    rqBuf[0] = 0x02;// code
    memcpy(rqBuf+1, &sendWhichChunk, 2);
    memcpy(rqBuf+3, &sendFileId, 2);
    
    if (send(sock, rqBuf, sizeof(rqBuf), 0) < 0)
        return false;
    for (int n = 0; n < 5; n++)
    {
        if (n == 3)
        {
            send(sock, rqBuf, sizeof(rqBuf), 0);
            printf("Resending request...\n");
        }
        
        int recvSize = recv(sock, buf, chunkSize+5, MSG_DONTWAIT);
        if (recvSize == 0)
        	continue;
        if (recvSize < 0)
        	return false;
        
        if (recvSize != chunkSize+5 && n == 4)
        {
        	printf("Giving up on current chunk (%hu)\n", whichChunk);
            return false;
        }
        
        uint16_t rChunk, rFileId;
        memcpy(&rChunk, buf+1, 2);
        memcpy(&rFileId, buf+3, 2);
        if (ntohs(rChunk) != whichChunk || ntohs(rFileId) != fileId || buf[0] != 0x02)
        {
            if (buf[0] == 0x02)
            {
                printf("Wrong chunk: %hu, %hu\n", ntohs(rChunk), ntohs(rFileId));
            }
            else printf("Invalid packet.\n");
            n--;
            continue;
        }
        if (recvSize == chunkSize+5) break;
    }
    
    return true;
}

bool getFrame(int sock, u8* fb)
{
    char fileUpdateRqCode = 0x01;
    printf("Sending file update request...\n");
    int bytesSent = send(sock, &fileUpdateRqCode, 1, 0);
    if (bytesSent < 0)
    {
        printf("Error sending packet: %s\n", strerror(errno));
        return false;
    }
    if (!bytesSent) return false;
    printf("Done\n");
        
    uint16_t fileSize = 0, chunkSize = 0, chunkCount = 0, fileId = 0;
        
    printf("Receiving file header...\n");
    int attempts;
    for (attempts = 0; attempts < 50; attempts++)
    {
    	usleep(32000);
        printf("Loop (%d)\n", errno);
        char recvBuf[9];
        memset(recvBuf, 0xFF, 9);
        int recvSize = recv(sock, recvBuf, 9, MSG_DONTWAIT);
        if (recvSize < 0)
        {
            printf("Failed to receive file update. (%d == %s)\n", errno, strerror(errno));
            return false;
        }
        if (recvSize == 9 && recvBuf[0] == 0x01)
        {
            printf("Successfully received file header\n");
            memcpy(&fileSize, recvBuf+1, 2);
            fileSize = ntohs(fileSize);
            memcpy(&chunkSize, recvBuf+3, 2);
            chunkSize = ntohs(chunkSize);
            memcpy(&chunkCount, recvBuf+5, 2);
            chunkCount = ntohs(chunkCount);
            memcpy(&fileId, recvBuf+7, 2);
            fileId = ntohs(fileId);
            break;
        }
        else if (recvSize > 0)
        {
            printf("Received unknown packet of %d bytes\n", recvSize);
            return false;
        }
        if (recvSize < 0)
        	continue;
    }
    printf("Done\n");
    printf("Got file header: %hu, %hu, %hu, %hu\n", fileSize, chunkSize, chunkCount, fileId);
        
    char* fileBuf = (char*)malloc(chunkCount*chunkSize);
    if (!fileBuf)
    {
        printf("FATAL: malloc() failed. (%d)\n", errno);
        exit(1);
    }
    bool chunks[chunkCount];
    memset(chunks, 0, sizeof(chunks));
        
    printf("Receiving chunks...\n");
    uint16_t nextChunk;
    int tries = -1;
    while (!allChunks(chunks, chunkCount, &nextChunk) && tries < chunkCount * 5)
    {
    	tries++;
        char* chunkBuf = (char*)malloc(chunkSize+5);
        if (!chunkBuf)
            exit(0xbeef03);
        memset(chunkBuf, 0xEE, 5);
        printf("Requesting chunk #%d...\n", nextChunk);
        chunks[nextChunk] = requestChunk(sock, chunkBuf, chunkSize, nextChunk, fileId);
        if (chunks[nextChunk])
        {
            printf("Success\n");
            memcpy(fileBuf+(nextChunk*chunkSize), chunkBuf+5, chunkSize);
        }
        else printf("Failure\n");
        free(chunkBuf);
    }
    printf("Finished receiving chunks.\n");
        
    if (allChunks(chunks, chunkCount, NULL))
    {
    	printf("Decoding...\n");
    	bool decodeFail = false;
		if (njDecode(fileBuf, fileSize))
		{
        	free(fileBuf);
        	printf("Error decoding jpeg\n");
        	decodeFail = true;
    	}
    	printf("Done\n");
        free(fileBuf);
        
        printf("Copying fb...\n");
        if (njGetWidth() == 240 && njGetHeight() == 320
            && njIsColor() && !decodeFail)
        {
        	memcpy(fb, njGetImage(), njGetImageSize());
        }
        else printf("Error: Received image not 240x320\n");
        printf("Done\n");
        
        njDone();
        
        u16 sendFileId = htons(fileId);
        char fileReceivedBuf[3];
        fileReceivedBuf[0] = 0x03;//code
        memcpy(fileReceivedBuf+1, &sendFileId, 2);
        send(sock, fileReceivedBuf, 3, 0);
        
        return true;
    }
    
    return false;
}

void sendMouseEvent(int sock, s16 x, s16 y, char click)
{
    char sendCode = 0x4;
    int outX, outY;
    outX = htons(x);
    outY = htons(y*-1);
    
    printf("Mouse: (%hhd, %hd, %hd, %hhd)\n", sendCode, x, y, click);
    
    char sendBuf[1+2+2+1];
    sendBuf[0] = sendCode;
    memcpy(sendBuf+1, &outX, 2);
    memcpy(sendBuf+1+2, &outY, 2);
    sendBuf[1+2+2] = click;
    
    send(sock, sendBuf, sizeof(sendBuf), 0);
}

int main(int argc, char **argv)
{
    gfxInitDefault();
    atexit(gfxExit);
	consoleInit(GFX_TOP, NULL);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	memset(fb, 0, FB_SIZE);
	printf("Started. Let's go.\n");
	atexit(pauseExit);
	
	
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	if(SOC_buffer == NULL)
	{
		failExit("memalign failed to allocate");
	}
	// Now intialise soc:u service
	if (socInit(SOC_buffer, SOC_BUFFERSIZE) != 0)
	{
    	failExit("socInit failed");
	}
	// register socShutdown to run at exit
	atexit(socShutdown);
	

	int sock;
    struct sockaddr_in sa;
    //socklen_t saLen;

    // create an Internet, datagram, socket using UDP
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        /* if socket failed to initialize, exit */
        failExit("Error Creating Socket");
    }

    memset(&sa, 0, sizeof(sa));// Zero out socket address
    
    sa.sin_family = AF_INET;// The address is IPv4
    sa.sin_addr.s_addr = inet_addr("10.0.0.2");// IPv4 adresses is a uint32_t, convert a string representation of the octets to the appropriate value
    sa.sin_port = htons(55550);// sockets are unsigned shorts, htons(x) ensures x is in network byte order, set the port to 55554
    //bytes_sent = sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&sa, sizeof(sa));
    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)))
        failExit("Connect Failed");
        
        
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
	socShutdown();
	printf("return 0...\n");
	return 0;
}
