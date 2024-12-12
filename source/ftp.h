#ifndef FTP_H
#define FTP_H

extern FS_Archive sdmcArchive;
extern char currentPath[];
extern u32 currentIP;
extern int dataPort;

void ftp_init();
void socShutdown();

int ftp_frame(int s);
int ftp_getConnection();

int ftp_openCommandChannel();
int ftp_openDataChannel();
int ftp_sendResponse(int s, int n, char* mes);

#endif
