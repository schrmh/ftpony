ftpony
======

the crappiest FTP server you've ever seen, now on 3DS

code by smea, icon by @Koopako

======  
Updated 2024 to make it compile...  
Added a quote by myself.  
No idea if it really works correctly...  
... listing doesn't but I guess that was broken before as well?  
(https://github.com/smealum/ftpony/issues/9)  

Apparently the LIST crash happens when this line is there unmodified:  
int ret=accept(listenfd, (struct sockaddr*)NULL, NULL);
