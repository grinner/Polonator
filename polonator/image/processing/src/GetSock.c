// =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Release 1.0 -- 02-12-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
//
/* GetSock.c
-
- Adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- Greg Porreca (Church Lab) 11-19-2007
- 
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Polonator_logger.h"

int GetSock(char *argv,
	    int port){

  int sock;                      // socket descriptor
  struct sockaddr_in servAddr;   // server address
  unsigned short servPort;       // server port
  char *servIP;                  // server IP address
  char errorString[255];
  struct hostent *h;
  struct in_addr a;
  int flag = 1;
  int result;

  int waiting_to_connect = 0;

  servIP = argv;
  servPort = port;
  
  fprintf(stdout, "GetSock:\tConnecting to server %s on port %d\n", servIP, servPort);
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    perror(NULL);
    fprintf(stdout, "ERROR: socket() failed\n");
    exit(1);
  }
  
  fprintf(stdout, "GetSock:\tResolving hostname...\n");
  h=gethostbyname(servIP);
  bcopy(*h->h_addr_list, (char*) &a, sizeof(a));
  fprintf(stdout, "GetSock:\tHost address: %s\n", inet_ntoa(a));


  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(a));
  //  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort);

  result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
  if(result<0){
    p_log_errorno((char*)"ERROR SETTING TCP_NODELAY ON SOCKET");
  }
      
  while(connect(sock, (struct sockaddr *) & servAddr, sizeof(servAddr)) < 0){
    if(waiting_to_connect){;}
    else{
      fprintf(stdout, "GetSock:\tWaiting to connect to server ...\n");
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
  
//
// ------------------------------------ END GetSock.c------------------------------------------
//
