/* =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// GetSock.c: opens and returns a TCP socket
//
// Release 1.0 -- 02-12-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/
/* get_sock.c
-
- Adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- Greg Porreca (Church Lab) 11-19-2007
- 
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"

int GetSock(char *argv,
	    int port){

    int sock;                      /* socket descriptor */
    struct sockaddr_in servAddr;   /* server address */
    unsigned short servPort;       /* server port */
    char *servIP;                  /* server IP address */
    char errorString[255];
    struct hostent *h;
    struct in_addr a;

    char log_string[500];

    int waiting_to_connect = 0;

    servIP = argv;
    servPort = port;

    sprintf(log_string, "STATUS:\tGetSock: Connecting to server %s on port %d", servIP, servPort);
    p_log(log_string);
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        p_log_errorno("ERROR:\tGetSock: socket() failed");
        exit(1);
    }

    p_log("STATUS:\tGetSock: Resolving hostname...");
    h=gethostbyname(servIP);
    bcopy(*h->h_addr_list, (char*) &a, sizeof(a));
    sprintf(log_string, "STATUS:\tGetSock: Host address: %s", inet_ntoa(a));
    p_log(log_string);


    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(a));
    servAddr.sin_port = htons(servPort);


    while(connect(sock, (struct sockaddr *) & servAddr, sizeof(servAddr)) < 0)
    {
        if(waiting_to_connect){;}
        else{
            p_log("STATUS:\tGetSock: Waiting to connect to server ...");
            waiting_to_connect = 1;
        }
    }
    /*
    if(connect(sock, (struct sockaddr *) & servAddr, sizeof(servAddr)) < 0){
    perror(NULL);
    DieWithError("connect() failed");
    }
    */

    return sock;

}
  
/*
// ------------------------------------ END GetSock.c---------------------------
/*/
