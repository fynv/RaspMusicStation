#ifndef _GetIP_h
#define _GetIP_h

#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

bool GetIP(unsigned char ip[4])
{
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) 
		{ 
			tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

			sscanf(addressBuffer,"%d.%d.%d.%d",ip,ip+1,ip+2,ip+3);
			if (*(int*)ip != 127+(1<<24)) return true;
        } 
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
    return false;
}



#endif