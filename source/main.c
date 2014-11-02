#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <3ds.h>

#include "costable.h"
#include "text.h"
#include "ftp.h"

char superStr[8192];
int cnt;

char* quotes[]={"\"wow this is the worst thing i've seen in a while\"\n",
				"\"<Namidairo> that hurts my brain\"\n",
				"\"<mtheall> let's make it like really good code\"\n",
				"\"//this code is not meant to be readable\"\n",
				"\"// i just found it like this\"\n"};
const int numQuotes = sizeof(quotes)/sizeof(*quotes);
int curQuote;

s32 pcCos(u16 v)
{
	return costable[v&0x1FF];
}

int countLines(char* str)
{
	if(!str)return 0;
	int cnt; for(cnt=1;*str=='\n'?++cnt:*str;str++);
	return cnt;
}

void cutLine(char* str)
{
	if(!str || !*str)return;
	char* str2=str;	for(;*str2&&*(str2+1)&&*str2!='\n';str2++);	str2++;
	memmove(str,str2,strlen(str2)+1);
}

void drawFrame()
{
	u8* bufAdr=gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	int i, j;
	for(i=1;i<400;i++)
	{
		for(j=1;j<240;j++)
		{
			u32 v=(j+i*240)*3;
			bufAdr[v]=(pcCos(i+cnt)+4096)/32;
			bufAdr[v+1]=(pcCos(j-256+cnt)+4096)/64;
			bufAdr[v+2]=(pcCos(i+128-cnt)+4096)/32;
		}
	}
	gfxDrawText(GFX_TOP, GFX_LEFT, NULL, "ftPONY v0.0003 gamma\n", 240-fontDefault.height*1, 10);
	u32 ip = gethostid();
	char bof[256];
	sprintf(bof, "IP: %d.%d.%d.%d\n", ip & 0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
	gfxDrawText(GFX_TOP, GFX_LEFT, NULL, bof, 240-fontDefault.height*2, 10);

	gfxDrawText(GFX_TOP, GFX_LEFT, NULL, quotes[curQuote], 240-fontDefault.height*3, 10);
	i = countLines(superStr);
	while(i>240/fontDefault.height-3){cutLine(superStr);i--;}
	gfxDrawText(GFX_TOP, GFX_LEFT, NULL, superStr, 240-fontDefault.height*4, 20);
	cnt++;

	gfxFlushBuffers();
	gfxSwapBuffers();
}

int main()
{
	srvInit();	
	aptInit();
	hidInit(NULL);
	irrstInit(NULL);
	gfxInit();

	gfxSet3D(false);

	srand(svcGetSystemTick());

	curQuote=rand()%numQuotes;
	superStr[0]=0;
	ftp_init();

	int connfd=ftp_getConnection();

	APP_STATUS status;
	while((status=aptGetStatus())!=APP_EXITING)
	{
		if(status == APP_RUNNING)
		{
			ftp_frame(connfd);
			drawFrame();

			hidScanInput();
			if(hidKeysDown()&KEY_B)break;
		}
		else if(status == APP_SUSPENDING)
		{
			aptReturnToMenu();
		}
		else if(status == APP_SLEEPMODE)
		{
			aptWaitStatusEvent();
		}
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	}

	ftp_exit();
	gfxExit();
	irrstExit();
	hidExit();
	aptExit();
	srvExit();
	return 0;
}
