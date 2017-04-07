#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <3ds.h>

#include "global.h"
#include "video.h"
#include "net.h"
#include "ini.h"


#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000


static u32 *SOC_buffer = NULL;

int sock;
struct sockaddr_in sa;


void net_init()
{
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


    // create an Internet, datagram, socket using UDP
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        /* if socket failed to initialize, exit */
        failExit("Error Creating Socket");
    }

    memset(&sa, 0, sizeof(sa));// Zero out socket address
    
    sa.sin_family = AF_INET;// The address is IPv4
    char* serverIpAddress = ini_getString("serveraddr");
    if (serverIpAddress)
        sa.sin_addr.s_addr = inet_addr("10.0.0.2");
    else
        failExit("Error: \"serveraddr\" MUST be within the .ini file!\n");
    free(serverIpAddress);
    sa.sin_port = htons(ini_getInt("port"));
    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)))
        failExit("connect() Failed");
}

void net_exit()
{
    printLog(0, "net_exit()...\n");
    close(sock);
    printLog(0, "waiting for socExit...\n");
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
    for (int n = 0; n < 3; n++)
    {
        usleep(10000);
        /*if (n == 3)
        {
            send(sock, rqBuf, sizeof(rqBuf), 0);
            printf("Resending request...\n");
        }*/
        
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

bool getFrame(int sock, u8* fb, int s_w, int s_h)
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
    unsigned char fileType = 0;
        
    printf("Receiving file header...\n");
    int attempts;
    for (attempts = 0; attempts < 30; attempts++)
    {
    	usleep(50000);
        printf("Loop (%d)\n", errno);
        char recvBuf[10];
        memset(recvBuf, 0xFF, 10);
        int recvSize = recv(sock, recvBuf, 10, MSG_DONTWAIT);
        if (recvSize < 0)
        {
            printf("Failed to receive file update. (%d == %s)\n", errno, strerror(errno));
            return false;
        }
        if (recvSize == 10 && recvBuf[0] == 0x01)
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
            fileType = recvBuf[9];
            break;
        }
        else if (recvSize > 0)
        {
            printLog(2, "Received unknown packet of %d bytes\n", recvSize);
            return false;
        }
        if (recvSize < 0)
        	continue;
    }
    printf("Done\n");
    printf("Got file header: %hu, %hu, %hu, %hu, %hhu\n", fileSize, chunkSize, chunkCount, fileId, fileType);
        
    char* fileBuf = (char*)malloc(chunkCount*chunkSize);
    if (!fileBuf)
    {
        printf("FATAL: malloc() failed. (%d)\n", errno);
        failExit("Malloc failed");
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
        u16 sendFileId = htons(fileId);
        char fileReceivedBuf[3];
        fileReceivedBuf[0] = 0x03;//code
        memcpy(fileReceivedBuf+1, &sendFileId, 2);
        send(sock, fileReceivedBuf, 3, 0);
        
        bool decodeSuccess = true;
    	printf("Decoding...\n");
    	
    	if (fileType == FTYPE_JPG)
    	{
    	    decodeSuccess = !video_decodeJpg(fileBuf, fileSize, fb, s_w, s_h);
    	}
    	else if (fileType == FTYPE_PNG)
    	{
    	    decodeSuccess = !video_decodePng(fileBuf, fileSize, fb, s_w, s_h);
    	}
    	
    	printf("Done.\n");
    	free(fileBuf);
        
        return decodeSuccess;
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
