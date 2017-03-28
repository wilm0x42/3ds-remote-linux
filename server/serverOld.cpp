#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>


#define DISCONNECTED 0
#define CONNECTED 1

struct host_t
{
    IPaddress ip;
    TCPsocket sock;
    Uint16 port;
};

struct client_t
{
    int status;
    IPaddress* ip;
    TCPsocket sock;
};


#define log printf

void init(host_t& host)
{
    /* initialize SDL */
	if(SDL_Init(0) == -1)
	{
		printf("SDL_Init: %s\n",SDL_GetError());
		exit(1);
	}

	/* initialize SDL_net */
	if(SDLNet_Init() == -1)
	{
		printf("SDLNet_Init: %s\n", SDLNet_GetError());
		exit(2);
	}
	
	/* Resolve the argument into an IPaddress type */
	host.port = 55554;
	if (SDLNet_ResolveHost(&host.ip, NULL, host.port) == -1)
	{
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		exit(3);
	}
	
	/* open the server socket */
	host.sock = SDLNet_TCP_Open(&host.ip);
	if (!host.sock)
	{
		printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		exit(4);
	}
}


void disconnectClient(client_t& client)
{
    SDLNet_TCP_Close(client.sock);
    client.status = DISCONNECTED;
    log("Disconnecting client %u\n", client.ip->host);
}


void handleConnections(client_t& client, host_t& host)
{    
    // try to accept a connection
    client.sock = SDLNet_TCP_Accept(host.sock);
        
    if (!client.sock)
    { // no connection accepted
        return;
    }

    // get the clients IP and port number
    client.ip = SDLNet_TCP_GetPeerAddress(client.sock);
    if (!client.ip)
    {
        printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
        return;
    }
		
    client.status = CONNECTED;
    printf("Accepted a connection\n");
}


void sendFrame(client_t&client)
{
    FILE* fp = fopen("frame.png", "r");
    if (!fp)
    {
    	system("./getScreencap.sh");
    	fp = fopen("frame.png", "r");
    }
    fseek(fp, 0, SEEK_END);
    long int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* frameBuf = (char*)malloc(fsize);
    if (!frameBuf) return;
    fread(frameBuf, 1, fsize, fp);
    
    int sendSize;
    SDLNet_Write32(fsize, &sendSize);
    
    SDLNet_TCP_Send(client.sock, &sendSize, sizeof(sendSize));
    SDLNet_TCP_Send(client.sock, frameBuf, fsize);
    
    system("./getScreencap.sh");
    free(frameBuf);
}


void doMouseEvent(client_t client)
{
    int recvInt;
    int mx, my;
    char mouse;
    printf("Getting mouseevent info...\n");
    SDLNet_TCP_Recv(client.sock, &recvInt, 4);
    mx = SDLNet_Read32(&recvInt);
    SDLNet_TCP_Recv(client.sock, &recvInt, 4);
    my = SDLNet_Read32(&recvInt);
    SDLNet_TCP_Recv(client.sock, &mouse, 1);
    printf("Done (%d, %d, %hhd)\n", mx, my, mouse);
        
    char clickcmd[80];
    snprintf(clickcmd, 80, "%s%s%s", (mouse==1)?"\"mouseclick 1\"":"", (mouse==2)?"\"mouseclick 1\" \"mouseclick 1\"":"",
                                     (mouse==3)?"\"mouseclick 3\"":"");
    char cmd[256];
    snprintf(cmd, 256, "xte \"mousermove %d %d\" %s", mx, my, clickcmd);
    system(cmd);
}


int main(int argc, char **argv)
{
    host_t host;
    client_t client;
    memset(&client, 0, sizeof(client_t));
    
    init(host);

	while (1)
	{
        SDL_Delay(100);
        
        if (client.status == DISCONNECTED)
        {
            handleConnections(client, host);
            continue;
        }

            
        if (client.status == CONNECTED)
        {
            char recvCode = 0;
            int received = SDLNet_TCP_Recv(client.sock, &recvCode, 1);
            if (received < 1) continue;
            printf("Received %d\n", recvCode);
                
            switch (recvCode)
            {
            case 1:
                printf("Sending frame...\n");
                sendFrame(client);
                printf("Done\n");
                break;
            case 2:
                disconnectClient(client);
                printf("Disconnected client\n");
                exit(0);
                break;
            case 3:
                doMouseEvent(client);
                break;
            default:
                printf("Unknown command code.\n");
                break;
            }
        }
        SDL_Delay(100);
	}

	/* shutdown SDL_net */
	SDLNet_Quit();

	/* shutdown SDL */
	SDL_Quit();

	return(0);
}


/* *********** SCRATCH PAD *********** */

/* print out the clients IP and port number */
		/*ipaddr = SDL_SwapBE32(remoteip->host);
		printf("Accepted a connection from %d.%d.%d.%d port %hu\n",
			ipaddr >> 24,
			(ipaddr >> 16) & 0xff,
			(ipaddr >> 8) & 0xff,
			ipaddr & 0xff,
			remoteip->port);*/
