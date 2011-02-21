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
/* receive_filename.c
-
- Network code adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- Receives filename of current set of images from server.  Called by
- initialize_processor.c and processor.c
-
- Written by Greg Porreca (Church Lab) 11-19-2007
- 
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "processor.h"

#define LEN 6

void ReceiveFilename(char *argv, int portnum,
		     char *filename)
{

	int sock;                      // socket descriptor
	int bytesRcvd, totalBytesRcvd=0; // bytes read in single rcv() and total bytes read
	char errorString[255];
	char sendBuffer[] = "2"; //CODE OF 2 ASKS SENDER FOR FILENAME

	sock = GetSock(argv, portnum);

	//SEND DIFFERENT CODE TO SERVER TO GET FILENAME

	bytesRcvd = 0;
	totalBytesRcvd = 0;

	// tell server we're ready for the filename
	if(send(sock, sendBuffer, 1, 0) != 1)
	{
		p_log_errorno((char*)"send() from client to request data failed");
	}

	// wait for full block to be received
	while(totalBytesRcvd < 6)
	{
		if((bytesRcvd = recv(sock, ((char*)filename) + totalBytesRcvd, LEN-totalBytesRcvd, 0)) <= 0)
		{
			sprintf(errorString, "recv() failed or connection closed prematurely, %d bytes received", totalBytesRcvd);
			p_log_errorno(errorString);
			close(sock);
			exit(0);
		}
		totalBytesRcvd += bytesRcvd;
	}
	if(shutdown(sock, 2) < 0)
	{
		perror(NULL);
	}//close(sock);
	*(filename + LEN) = 0;
}
  
//
// ------------------------------------ END ReceiveFilename.c------------------------------------------
//
