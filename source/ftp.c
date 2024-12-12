#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <3ds.h>
#include <fcntl.h>

#include "output.h"
#include "ftp.h"
#include "ftp_cmd.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define ARCHIVE_SDMC    0x00000009 ///< SDMC archive.
//FS_Path fsMakePath(FS_PathType type, const void* path);

static u32 *SOC_buffer = NULL;

void failExit(const char *fmt, ...);

FS_Archive archive = 0;

char tmpBuffer[512];
const int commandPort=5000;
int dataPort=5001;
char currentPath[4096];
u32 currentIP;

int listenfd;

void ftp_init()
{
	Result ret;
	ret=fsInit();
	print("fsInit %08X\n", (unsigned int)ret);
	
	FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	print("FSUSER_OpenArchive %08X\n", (unsigned int)ret);
	
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		failExit("memalign: failed to allocate\n");
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    	failExit("socInit: 0x%08X\n", (unsigned int)ret);
	}

	// register socShutdown to run at exit
	// atexit functions execute in reverse order so this runs before gfxExit
	atexit(socShutdown);
	
	sprintf(currentPath, "/");

	currentIP=(u32)gethostid();

	listenfd=-1;
}

void socShutdown()
{
	socExit();
}

int ftp_openCommandChannel()
{
	if(listenfd<0)
	{
		struct sockaddr_in serv_addr;

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(commandPort); 

		bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
		fcntl(listenfd, F_SETFL, O_NONBLOCK);

		listen(listenfd, 10); 
	}
	
	int ret=accept(listenfd, (struct sockaddr*)NULL, NULL);
	if(ret>=0)
	{
		closesocket(listenfd);
		listenfd=-1;
		fcntl(ret, F_SETFL, O_NONBLOCK);
	}

	return ret;
}

int ftp_openDataChannel()
{
	int listenfd;
	struct sockaddr_in serv_addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(dataPort); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 
	
	int ret=accept(listenfd, (struct sockaddr*)NULL, NULL);
	closesocket(listenfd);

	return ret;
}

int ftp_sendResponse(int s, int n, char* mes)
{
	char data[128];
	sprintf(data, "%d %s\r\n", n, mes);
	return send(s,data,strlen(data),0);
}

int linelen(char* str)
{
	int i=0; while(*str && *str!='\n' && *str!='\r'){i++;str++;}
	*str=0x0;
	return i;
}

int ftp_processCommand(int s, char* data)
{
	if(!data)return -1;
	int n=linelen(data);
	char cmd[5];
	char arg[256]="";
	if(n>2 && (!data[3] || data[3]==' ' || data[3]=='\n' || data[3]=='\r')){memcpy(cmd,data,3);cmd[3]=0x0; if(n>3)memcpy(arg, &data[4], n-4);}
	else if(n>3 && (!data[4] || data[4]==' ' || data[4]=='\r' || data[4]=='\n')){memcpy(cmd,data,4);cmd[4]=0x0; if(n>4)memcpy(arg, &data[5], n-5);}
	else return -1;

	print("\nreceived command : %s (%s)",cmd,arg);

	int i;
	for(i=0; i<ftp_cmd_num; i++)if(!strcmp(cmd, ftp_cmd[i].name)){ftp_cmd[i].handler(s, cmd, arg); break;}
	if(i>=ftp_cmd_num)ftp_sendResponse(s, 502, "invalid command");
	return 0;
}

int ftp_frame(int s)
{
	char buffer[512];
	memset(buffer, 0, 512);
	int ret=recv(s,buffer,512,0);
	if(!ret)return 1; //client has disconnected
	else return ftp_processCommand(s,buffer);
}

int ftp_getConnection()
{
	int connfd = ftp_openCommandChannel();
	if(connfd>=0)
	{
		print("received connection ! %d\ngreeting...",connfd);
		ftp_sendResponse(connfd, 200, "hello");
	}
	return connfd;
}
