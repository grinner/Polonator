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
/* SendData.c
-
- Adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- Server to send data blocks and filenames; each is a
- 2MB image composed of 1e6 2-byte pixels or a 6-char filename.  
- Attaches an 8-byte 4-element header to each block for use in the
- image-processing pipeline.
-
- Need to 'upgrade' in future by forking off a process for each
- connection to support multiple CPUs on the processing side.
-
- Greg Porreca (Church Lab) 11-19-2007
- 
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ProcessorParams.h"
#include "SendData.h"

#define MAXPENDING 5

int main(int argc, char *argv[])
{
	int servSock;                  // socket descriptor: server
	int clntSock;                  // socket descriptor: client
	struct sockaddr_in servAddr;   // server address
	struct sockaddr_in clntAddr;   // client address
	unsigned short servPort;       // server port
	unsigned int clntLen;          // length of client address data structure

	short unsigned int *image;
	int i, return_val;
	FILE *imgfile;


	image = (short unsigned int*)malloc(1000004*sizeof(short unsigned int));

	/*
	if( (imgfile=fopen("images/pm1/SC_0009.raw", "rb")) == NULL){
	//if( (imgfile=fopen("images/999/WL_0003.raw", "rb")) == NULL){
	perror(NULL);
	exit(1);
	}

	for(i=0; i<4; i++){
	*(image+i)=1;
	}
	fread(image+4, sizeof(short unsigned int), 1000000, imgfile);
	fclose(imgfile);
	*/

	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		exit(1);
	}

	servPort = PORTNUM;//atoi(argv[1]);

	while(1)
	{
		if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		{
			perror(NULL);
			DieWithError("socket() failed");
		}

		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAddr.sin_port = htons(servPort);
		if(setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, "TRUE", 4) != 0)
		{
			perror(NULL);
		};
		if(bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		{
			perror(NULL);
			DieWithError("bind() failed");
		}
    
		if(listen(servSock, MAXPENDING) < 0)
		{
			perror(NULL);
			DieWithError("listen() failed");
		}
		clntLen = sizeof(clntAddr);
    
		if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntLen)) < 0)
		{
			DieWithError("accept() failed");
		}
		fprintf(stdout, "Handling client %s:%d\n", inet_ntoa(clntAddr.sin_addr), servPort);


		return_val = HandleSendRequest(clntSock, image, argv[1]); // will return 0 when finished sending data
		fprintf(stdout, "SendData: close connection\n");

		if(shutdown(servSock,2)<0){
			perror(NULL);
		}
		/*    if(return_val == 0){
		  free(image);
		  exit(1);
		  }*/
	}
}

//
// ------------------------------------ END SendData.c------------------------------------------
//
