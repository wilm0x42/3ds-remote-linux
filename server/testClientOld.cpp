#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_image.h>

void recvFrame(TCPsocket sock, char* fb)
{
    char sendCode;
    int recvCode;
    int frameSize;
    sendCode = 1;
    printf("Sending command code...\n");
    SDLNet_TCP_Send(sock, &sendCode, sizeof(sendCode));
    printf("Done\n");
    
    printf("Waiting on frame size...\n");
    SDLNet_TCP_Recv(sock, &recvCode, sizeof(recvCode));
    frameSize = SDLNet_Read32(&recvCode);
    printf("Done (%d)\n", frameSize);
    
    printf("Allocating buffer...\n");
    char* buf = (char*)malloc(frameSize);
    if (!buf) return;
    memset(buf, 0x00, frameSize);
    printf("Done\n");
    
    printf("Receiving frame...\n");
    //int frameReceived = 0;
    //int tries;
    //for (tries = 0; tries < frameSize/32; tries++)
    //{
        int received = SDLNet_TCP_Recv(sock, buf, frameSize);
    //}
    FILE* dump = fopen("dump.png", "w");
    fwrite(buf, 1, frameSize, dump);
    fclose(dump);
    
    printf("Done\n");
    printf("Creating RWops from mem...\n");
    SDL_RWops* rwop = SDL_RWFromMem(buf, frameSize);
    printf("Done\n");
    printf("Decoding PNG...\n");
    SDL_Surface* surf = IMG_Load_RW(rwop, 0);
    printf("Format: %s\n", SDL_GetPixelFormatName(surf->format->format));
    printf("Pitch: %d\n", surf->pitch);
    printf("Done\n");
    printf("Closing RWops..\n");
    SDL_RWclose(rwop);
    printf("Done\n");
    if (surf == NULL)
    {
        printf("Error: returned surface is NULL\n");
        free(buf);
        return;
    }
    if (surf->w != 240 || surf->h != 320)
    {
        printf("Error: IMG size != 240x320\n");
        free(buf);
        return;
    }
    printf("Copying...\n");
    memcpy(fb, surf->pixels, 320*240*3);
    printf("Done\n");
    printf("Freeing surf...\n");
    SDL_FreeSurface(surf);
    printf("Done\n");
}

void sendMouseEvent(TCPsocket sock, int x, int y, char click)
{
    char sendCode = 3;
    int outX, outY;
    SDLNet_Write32(x, &outX);
    SDLNet_Write32(y, &outY);
    
    printf("Sending: (%hhd, %d, %d, %hhd)\n", sendCode, x, y, click);
    
    SDLNet_TCP_Send(sock, &sendCode, 1);
    SDLNet_TCP_Send(sock, &outX, 4);
    SDLNet_TCP_Send(sock, &outY, 4);
    SDLNet_TCP_Send(sock, &click, 1);
}

int main(int argc, char **argv)
{
	IPaddress ip;
	TCPsocket sock;
	Uint16 port;
    int ProgramRunning = 1;

	/* check our commandline */
	if(argc<3)
	{
		printf("%s host port\n",argv[0]);
		exit(0);
	}

	/* initialize SDL */
	if(SDL_Init(0)==-1)
	{
		printf("SDL_Init: %s\n",SDL_GetError());
		exit(1);
	}

	/* initialize SDL_net */
	if(SDLNet_Init()==-1)
	{
		printf("SDLNet_Init: %s\n",SDLNet_GetError());
		exit(2);
	}

	/* get the port from the commandline */
	port=(Uint16) strtol(argv[2],NULL,0);

	/* Resolve the argument into an IPaddress type */
	if(SDLNet_ResolveHost(&ip,argv[1],port)==-1)
	{
		printf("SDLNet_ResolveHost: %s\n",SDLNet_GetError());
		exit(3);
	}

	/* open the server socket */
	sock=SDLNet_TCP_Open(&ip);
	if(!sock)
	{
		printf("SDLNet_TCP_Open: %s\n",SDLNet_GetError());
		exit(4);
	}
	
	SDL_Window* window = SDL_CreateWindow("client", 0, 0, 320*2, 240*2, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Event event;
	SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 240, 320);
	char fb[240*320*3];
	memset(fb, 0, sizeof(fb));
	
	IMG_Init(IMG_INIT_PNG);

	while (ProgramRunning == 1)
	{

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                ProgramRunning = 0;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    printf("Receiving frame...\n");
                    recvFrame(sock, fb);
                    printf("Done\n");
                }
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    char sendCode = 2;
                    SDLNet_TCP_Send(sock, &sendCode, 1);
                    SDLNet_TCP_Close(sock);
                    ProgramRunning = 0;
                }
                if (event.key.keysym.sym == SDLK_c)
                {
                    printf("Sending mouse event...\n");
                    sendMouseEvent(sock, 5, 5, 1);
                    printf("Done\n");
                }
            default:
                break;
            }
        }
        

        SDL_UpdateTexture(tex, NULL, fb, 240*3);
        SDL_Rect outRect = {80, -80, 240*2, 320*2};
        SDL_RenderCopyEx(renderer, tex, NULL, &outRect, -90, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
        SDL_Delay(10);
	}
	
	SDLNet_TCP_Close(sock);

	/* shutdown SDL_net */
	SDLNet_Quit();

	/* shutdown SDL */
	SDL_Quit();

	return(0);
}
