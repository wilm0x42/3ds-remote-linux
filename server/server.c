#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */
#include <stdlib.h>
#include <stdint.h>

#define CHUNKSIZE 600

#define FTYPE_JPG 0
#define FTYPE_PNG 1


char* fileData = NULL;
long int fileSize = 0;
uint16_t fileId = 0;
char fileType = FTYPE_JPG;


long int loadFile(char** buf, const char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        printf("Error: couldn't open file \"%s\"\n", filename);
        return 0;
    }
    
    fseek(fp, 0, SEEK_END);
    long int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (buf == NULL)
    {
        fclose(fp);
        return fsize;
    }
    
    *buf = (char*)malloc(fsize);
    if (!*buf)
    {
        fclose(fp);
        return 0;
    }
    if (fread(*buf, 1, fsize, fp) < fsize)
    {
        free(*buf);
        *buf = NULL;
        return 0;
    }
    
    fclose(fp);
    return fsize;
}

void updateFile()
{
    fileId++;
    free(fileData);
    /*char command[256];
    char fileParam[16];
    snprintf(fileParam, 16, "%d", fileType);
    strcpy(command, "./getScreencap.sh ");
    strncat(command, fileParam, 255);
    int ret = system(command);
    
    if (fileType == FTYPE_JPG)
        fileSize = loadFile(&fileData, "work/frame.jpg");
    else if (fileType == FTYPE_PNG)
        fileSize = loadFile(&fileData, "work/frame.png");*/
        
    fileType = FTYPE_PNG;
    int ret = system("./getScreencap.sh 1");
    fileSize = loadFile(NULL, "work/frame.png");
    
    if (fileSize > (15*1024))
    {
        printf("PNG is too big. Sending jpeg instead.\n");
        fileType = FTYPE_JPG;
        ret = system("./getScreencap.sh 0");
        fileSize = loadFile(&fileData, "work/frame.jpg");
    }
    else
    {
        fileSize = loadFile(&fileData, "work/frame.png");
    }
    
    if (!fileSize || !fileData)
        return;
}

int doUpdateFileRequest(int sock)
{
    uint16_t chunkCount = fileSize / CHUNKSIZE;
    if (fileSize % CHUNKSIZE) chunkCount++;
    
    char sendBuf[10];// u16 filesize, u16 chunksize, u16 chunkcount, u16 fileid, u8 fileType
    memset(sendBuf, 0, sizeof(sendBuf));
    uint16_t sendFilesize = htons(fileSize),
             sendChunksize = htons(CHUNKSIZE),
             sendChunkcount = htons(chunkCount),
             sendFileid = htons(fileId);
    sendBuf[0] = 0x01;// code
    memcpy(sendBuf+1, &sendFilesize, 2);
    memcpy(sendBuf+1+2, &sendChunksize, 2);
    memcpy(sendBuf+1+4, &sendChunkcount, 2);
    memcpy(sendBuf+1+6, &sendFileid, 2);
    sendBuf[9] = fileType;
    
   return send(sock, sendBuf, sizeof(sendBuf), 0);
}

int doChunkRequest(int sock, uint16_t rqChunk, uint16_t rqFileId)
{
    if (rqChunk > (fileSize / CHUNKSIZE) + 1)
        return 0;
    if (rqFileId != fileId)
    {
        printf("Err: Client requested fileId %d, but current is %d. Rejecting.\n", rqFileId, fileId);
        return 0;
    }
    
    char sendBuf[1+4+CHUNKSIZE];// u16 which_chunk, u16 fileid, u8 chunkdata[chunksize]
    uint16_t sendRqChunk = htons(rqChunk);
    uint16_t sendFileId = htons(rqFileId);
    sendBuf[0] = 0x02;// code
    memcpy(sendBuf+1, &sendRqChunk, 2);
    memcpy(sendBuf+1+2, &sendFileId, 2);
    
    int fileOffset = rqChunk * CHUNKSIZE;
    
    if (rqChunk <= (fileSize / CHUNKSIZE))
        memcpy(sendBuf+5, fileData+fileOffset, CHUNKSIZE);
    else
        memcpy(sendBuf+5, fileData+fileOffset, fileSize % CHUNKSIZE);
    
    return send(sock, sendBuf, sizeof(sendBuf), 0);
}

void getMouseStr(char m, char* buf, int maxlen)
{
    memset(buf, 0, maxlen);
    if (m & 0x01) strncat(buf, "\"mouseclick 1\" ", maxlen);//lclick
    if (m & 0x02) strncat(buf, "\"mouseclick 3\" ", maxlen);//rclick
    if (m & 0x04) strncat(buf, "\"mouseclick 1\" \"mouseclick 1\" ", maxlen);//double lclick
}

int main(int argc, char** argv)
{
    int sock;
    struct sockaddr_in sa;
    socklen_t fromlen;
    
    char recvBuf[8];
    ssize_t recvsize;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(55550);
    fromlen = sizeof(sa);

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        perror("error bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    if (!remove("work/prevframe.png"))
        printf("Removed work/prevframe.png\n");
    updateFile();

    printf("Waiting for client...\n");
    while (1)
    {
        printf("Loop\n");
        usleep(1000);
        recvsize = recvfrom(sock, (void*)recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&sa, &fromlen);
        if (recvsize < 0)
        {
            fprintf(stderr, "%d -- %s\n", errno, strerror(errno));
            exit(1);
        }
        if (recvsize == 0)
            continue;
            
        printf("Connecting... ");
        if (connect(sock, (struct sockaddr*)&sa, fromlen) < 0)
        {
            fprintf(stderr, "Connect failed: %s\n", strerror(errno));
            exit(1);
        }
        printf("%d\n", errno);
        
        switch (recvBuf[0])
        {
        case 0x01://file update request
        {
            printf("Fulfilling File Update Request\n");
            doUpdateFileRequest(sock);
            printf("Sent: %lu, %u, %lu, %hu, %hhu\n", fileSize, CHUNKSIZE, fileSize / CHUNKSIZE+1, fileId, fileType);
            printf("Done\n");
            break;
        }
        case 0x02://chunk request
        {
            uint16_t rqChunk, rqFileId;
            memcpy(&rqChunk, recvBuf+1, 2);
            memcpy(&rqFileId, recvBuf+3, 2);
            rqChunk = ntohs(rqChunk);
            rqFileId = ntohs(rqFileId);
            printf("Fulfilling chunk request... (%u)\n", rqChunk);
            doChunkRequest(sock, rqChunk, rqFileId);
            printf("Done\n");
            break;
        }
        case 0x03://File Received
        {
            printf("File was successfully received. Updating...\n");
            
            uint16_t recvFileId;
            memcpy(&recvFileId, recvBuf+1, 2);
            recvFileId = ntohs(recvFileId);
            if (recvFileId != fileId)//Only update file if client received CURRENT file
            {
                printf("Nvm.\n");
                break;
            }
            
            updateFile();
        
            printf("Done.\n");
            break;
        }
        case 0x04://Mouse event
        {
            char mouseCmdBuf[256];
            char clickBuf[80];
            short mouseX, mouseY;
            memcpy(&mouseX, recvBuf+1, 2);
            memcpy(&mouseY, recvBuf+3, 2);
            mouseX = ntohs(mouseX);
            mouseY = ntohs(mouseY);
            getMouseStr(recvBuf[5], clickBuf, 80);
            snprintf(mouseCmdBuf, 256, "xte \"mousermove %hd %hd\" %s", mouseX, mouseY, clickBuf);
            printf("MOUSE: %hd %hd\n", mouseX, mouseY);
            int ret = system(mouseCmdBuf);
            break;
        }
        case 0x05://Arrow key event
        {
            printf("Firing arrow keys: %02x\n", recvBuf[1]);
            char cmdBuf[256];
            memset(cmdBuf, 0, 256);
            strncat(cmdBuf, "xte ", 256);
            if (recvBuf[1] & 0x01) strncat(cmdBuf, "\"key Up\" ", 256);
            if (recvBuf[1] & 0x02) strncat(cmdBuf, "\"key Down\" ", 256);
            if (recvBuf[1] & 0x04) strncat(cmdBuf, "\"key Left\" ", 256);
            if (recvBuf[1] & 0x08) strncat(cmdBuf, "\"key Right\" ", 256);
            int ret = system(cmdBuf);
            break;
        }
        default:
            printf("Unknown message from client.\n");
            break;
        }
        printf("recvsize: %d\n ", (int)recvsize);
    }
}