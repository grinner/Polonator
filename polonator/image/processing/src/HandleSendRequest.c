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
/* HandleSendRequest.c
-
- Adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- 'Worker' method to send data blocks and filenames.  
-
- Greg Porreca (Church Lab) 11-19-2007
- 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "ProcessorParams.h"
#include "SendData.h"

#define LEN 1000000

int HandleSendRequest(int clntSocket,
		      short unsigned int *image,
		      char *filename)
{

	//  short unsigned int *image; // pixel data
	short unsigned int filenum; // 0 to N (# of frames)
	short unsigned int arraynum; // 0 to 17
	short unsigned int fcnum; // 0 or 1
	short unsigned int img_err_code; // 0=normal image, 1=dropped image

	FILE *imgfile;
	char imgfilename[200];
	char imgnum[8];

	char input[1];
	int i,j,k, m;

	int data_size = (LEN+4) * sizeof(short unsigned int);
	int count;

	// DETERMINE WHETHER FILENAME OR DATA IS CALLED FOR
	if (recv(clntSocket, input, 1, 0) < 0)
	{
		perror(NULL);
		DieWithError("recv() failed waiting for flag from client");
	}
	fprintf(stdout, "Received request from client: %c\n", *input);

	if (*input == '2')
	{
		//SEND FILENAME
		if(send(clntSocket, filename, strlen(filename)+1, 0) != strlen(filename)+1)
		{
			perror(NULL);
			fprintf(stdout, "%s\n", filename);
			DieWithError("send() failed");
			return 0;
		}
		return 1;
	}

	else if( (*input != '1') && (*input != '2') )
	{
		fprintf(stdout, "UNRECOGNIZED SEND REQUEST\n");
		//ERROR; UNRECOGNIZED REQUEST
	}

	else
	{
		//SEND DATA
		count=0;
		for(i=0; i < FCs_PER_RUN; i++)
		{
			for(j=0; j < ARRAYS_PER_FC; j++)
			{
				for(k=0; k < IMGS_PER_ARRAY; k++)
				{

					// wait for client to signal ready to receive data
					if (recv(clntSocket, input, 1, 0) < 0)
					{
						perror(NULL);
						DieWithError("recv() failed waiting for flag from client");
					}
				
					// ERIK--
					// DROP CODE IN HERE TO RETRIEVE POINTER TO CURRENT IMAGE
					// SHOULDNT MODIFY OUTSIDE THIS BLOCK

	  
					// construct header
					*(image + 0) = i; // filenum
					*(image + 1) = j; // array num
					*(image + 2) = k; // flowcell number
					*(image + 3) = 0; // no error

					// open file
					strcpy(imgfilename, "images/");
					strcat(imgfilename, filename);
					strcat(imgfilename, "/");
					sprintf(imgnum, "%07d", count++);
					strcat(imgfilename, imgnum);
					strcat(imgfilename, ".raw");
					fprintf(stdout, "%s\n", imgfilename);
					if( (imgfile=fopen(imgfilename, "rb")) == NULL)
					{
						perror(NULL);
						exit(1);
					}
					fread(image+4, sizeof(short unsigned int), 1000000, imgfile);
					fclose(imgfile);
					/*	  	  
					for(m=0; m < 1000000; m++)
					{
						*(image+i+4) = 16383 - *(image+i+4);
					}*/
	  

					// ERIK--
					// SHOULDNT MODIFY OUTSIDE THIS BLOCK
	  
					if(send(clntSocket, image, data_size, 0) != data_size)
					{
						perror(NULL);
						fprintf(stdout, "%d %d %d\n", i, j, k);
						DieWithError("send() failed");
						return 0;
					}

				}
			}
		}
		exit(1);
		//return 0; //tell SendData to exit
	}
}
//
// ------------------------------------ END HandleSendRequest.c------------------------------------------
//
