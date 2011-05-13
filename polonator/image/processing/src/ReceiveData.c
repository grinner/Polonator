// =============================================================================
//
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Release 1.0 -- 02-12-2008
// Release 1.1 -- 02-02-2009 modified to fix lost 'image send' command bug [GP]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
//
/* receive_data.c
-
- Network code adapted from http://cs.baylor.edu/~donahoo/practical/CSockets
-
- Connects to server and receives data blocks; each is a raw image plus a
- header.  Passes the image and header info to ProcessImage for image
- image processing, which consists of cross-correlation registration, data
- extraction, and output to disk.  Called by processor.c
-
- Written by Greg Porreca (Church Lab) 12-14-2007
-
*/

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "ProcessorParams.h"
#include "Polonator_logger.h"
#include "processor.h"

#ifdef SAVE_FL_IMAGES
extern char CYCLE_NAME[];
#endif

void ReceiveData(char *argv, int portnum,
		 short unsigned int *reg_pix_list_xcols,
		 short unsigned int *reg_pix_list_yrows,
		 FILE *full_pixel_list,
		 short unsigned int *beadvals,
		 FILE *beadfile,
		 FILE *sumfile,
		 FILE *imgsumfile,
		 FILE *reglogfile,
		 int curr_fcnum)
{

	int sock;                      // socket descriptor
	unsigned int data_size;        // length of data block to receive
	int bytesRcvd, totalBytesRcvd=0; // bytes read in single rcv() and total bytes read
	int imagesRcvd=0;
	char errorString[255];
	char log_string[255];
	char sendBuffer[] = "18"; //CODE OF 1/8 ASKS SENDER FOR DATA

	short unsigned int *inputBuffer;//
	short unsigned int img_err_code;

	int i, j, k;

	int *offsetx_history, *offsety_history;

	clock_t time1, time2;
	int num_loops;

	int connection_isok=1;
	char command_buffer[255];
    char image_dir[255];
    sprintf(image_dir, "%s/polonator/G.007/acquisition/images", getenv("HOME"));
    sprintf(command_buffer, "mkdir -p %s", image_dir);
    system(command_buffer);

#ifdef SAVE_FL_IMAGES
	char outputfilename[255];
	FILE *imgfile;
#endif

	sock = GetSock(argv, portnum);


	data_size = ((NUM_XCOLS * NUM_YROWS)+4) * sizeof(short unsigned int);
	if( (inputBuffer=(short unsigned int *)malloc(data_size)) == NULL)
	{
		p_log_errorno((char*)"malloc() failed");
	}

	//ALLOCATE MEMORY FOR OFFSET HISTORY ARRAYS (USED BY PROCESSIMAGE_REGISTER)
	if((offsetx_history=(int*)malloc(OFFSET_HISTORY_LENGTH * sizeof(int))) == NULL)
	{
		p_log_errorno((char*)"malloc() offsetx_history failed");
		exit(0);
	}
	if((offsety_history=(int*)malloc(OFFSET_HISTORY_LENGTH * sizeof(int))) == NULL)
	{
		p_log_errorno((char*)"malloc() offsety_history failed");
		exit(0);
	}
	  for(i=0; i < OFFSET_HISTORY_LENGTH; i++)
	  {
		*(offsetx_history + i) = 0;
		*(offsety_history + i) = 0;
	  }


  //TELL SENDER WE WANT TO RECEIVE DATA
	if(send(sock, sendBuffer, 1, 0)!=1)
	{
		if(errno = EPIPE)
		{
			p_log_errorno((char*)"ERROR:\tReceivData: send() failed because connection is broken");
			connection_isok=0;
		}
		else
		{
			p_log_errorno((char*)"send() from client to request data failed");
			exit(0);
		}
	}
	if(send(sock, sendBuffer+1, 1, 0) !=1 )
	{
		if(errno = EPIPE)
		{
			p_log_errorno((char*)"ERROR:\tReceivData: send() failed because connection is broken");
			connection_isok=0;
		}
		else
		{
			p_log_errorno((char*)"send() from client to request data failed");
			exit(0);
		}
	}

	//CONSUME 'PRIMIMG' DATA SINCE FIRST IMAGE XFER
	//IS VERY SLOW
	if(connection_isok)
	{
		bytesRcvd=0;
		totalBytesRcvd=0;
		while(totalBytesRcvd < data_size)
		{
			if((bytesRcvd=recv(sock, ((char*)inputBuffer)+totalBytesRcvd, data_size-totalBytesRcvd, 0)) < 0)
			{
				sprintf(errorString, "recv() failed or connection closed prematurely, %d bytes received (%d)", totalBytesRcvd, bytesRcvd);
				close(sock);
				p_log_errorno(errorString);
				exit(0);
			}
			else if(bytesRcvd == 0)
			{
				p_log((char*)"ERROR:\tReceiveData: connection is broken");
				connection_isok = 0;
				totalBytesRcvd=data_size;
			}
			totalBytesRcvd += bytesRcvd;
		}
	}

#ifdef SAVE_FL_IMAGES
	sprintf(outputfilename, "mkdir -p %s/%s", image_dir, CYCLE_NAME);
	system(outputfilename);
#endif

	// loop through all images, receiving blocks of data (1 image + header per block)
	if(connection_isok)
	{
		for(i=curr_fcnum; i < curr_fcnum + 1; i++)
		{
			for(j=0; j < ARRAYS_PER_FC; j++)
			{
				for(k=0; k < IMGS_PER_ARRAY; k++)
				{
					bytesRcvd = 0;
					totalBytesRcvd = 0;
					// tell server we're ready for the next block
#ifdef DEBUG1
					p_log((char*)"STATUS:\tReceiveData: request data from server");
					p_log((char*)"STATUS:\tReceiveData: send first byte to request");
#endif
					if(send(sock, sendBuffer, 1, 0) != 1)
					{
						if(errno = EPIPE)
						{
							p_log_errorno((char*)"ERROR:\tsend() failed because connection is broken");
							connection_isok=0;
							totalBytesRcvd=data_size;
						}
						else
						{
							p_log_errorno((char*)"ERROR:\tsend() from client to request data failed");
							exit(0);
						}
					}
#ifdef DEBUG1
					p_log((char*)"STATUS:\tReceiveData: send second byte to request");
#endif
					if(send(sock, sendBuffer+1, 1, 0) !=1 )
					{
						if(errno = EPIPE)
						{
							p_log_errorno((char*)"ERROR:\tsend() failed because connection is broken");
							connection_isok = 0;
							totalBytesRcvd = data_size;
						}
						else
						{
							p_log_errorno((char*)"ERROR:\tsend() from client to request data failed");
							exit(0);
						}
					}

#ifdef DEBUG1
					p_log((char*)"STATUS:\tReceiveData: start receiving data");
					p_log((char*)"STATUS:\tReceiveData: wait for data to arrive");
#endif
					// wait for full block to be received
					while(totalBytesRcvd < data_size)
					{
						// was there an error during the recv?  if so, this is bad; crash
						if((bytesRcvd = recv(sock, ((char*)inputBuffer) + totalBytesRcvd, data_size-totalBytesRcvd, 0)) < 0)
						{
							sprintf(errorString, "recv() failed, %d bytes received (%d)", totalBytesRcvd, bytesRcvd);
							close(sock);
							p_log_errorno(errorString);
							// do a graceful restart here
							connection_isok = 0;
							totalBytesRcvd = data_size;
							bytesRcvd = 0;
						}

						// was the connection broken?  if so, the acq software was probably stopped prematurely; recover gracefully
						else if(bytesRcvd == 0)
						{
							sprintf(log_string, "connection to acq appears to be broken while trying to receive image %d %d %d, received %d bytes so far", i, j, k, totalBytesRcvd);
							p_log(log_string);
							connection_isok = 0;
							totalBytesRcvd = data_size;
						}
						/*
#ifdef DEBUG1
						sprintf(log_string, "STATUS:\tReceiveData: received %d bytes of image data", bytesRcvd);
						p_log(log_string);
#endif
						*/
						totalBytesRcvd += bytesRcvd;
						num_loops++;
					}

					if(!connection_isok)
					{
						i = curr_fcnum + 1;
						j = ARRAYS_PER_FC;
						k = IMGS_PER_ARRAY;
						p_log((char*)"ERROR:\tReceiveData: connection was broken; exiting");
						break;
					}

					p_log((char*)"STATUS:\tReceiveData: all data received for current image");
					imagesRcvd++;
					num_loops = 0;

					// NOW THAT WE HAVE THE IMAGE, DO SOMETHING WITH IT
#ifdef SAVE_FL_IMAGES
					if( (k % 100) == 0)
					{
						sprintf(outputfilename, "%s/%s/%02d_%04d.raw",
								image_dir, CYCLE_NAME, *(inputBuffer+1), *(inputBuffer+2));
						sprintf(log_string, "STATUS:\tReceiveData: write received data to image file %s",
								outputfilename);
						p_log(log_string);
						imgfile = fopen(outputfilename, "w");
						fwrite(inputBuffer+4, sizeof(short unsigned int), 1000000, imgfile);
						fclose(imgfile);
					}
#endif

#ifdef DEBUG1
					sprintf(log_string, "STATUS:\tReceiveData: calling ProcessImage for frame %d %d %d, expecting %d %d %d",
							*(inputBuffer),
							*(inputBuffer+1),
							*(inputBuffer+2),
							i, j, k);
					p_log(log_string);
#endif
					ProcessImage(reg_pix_list_xcols,
						reg_pix_list_yrows,
						full_pixel_list,
						inputBuffer + 4,
						inputBuffer,
						beadvals,
						beadfile,
						sumfile,
						imgsumfile,
						reglogfile,
						offsetx_history,
						offsety_history);


				} // end for k
			} // end for j
		} // end for i
	} // end if connection_isok

	// tell acq we're finished receiving all data (we don't want it to close the connection
	// before the processor finishes receiving the last image)
	if(connection_isok)
	{
		if(send(sock, sendBuffer, 1, 0) != 1)
		{
			p_log_errorno((char*)"send() from client to signal all data received failed");
		//exit(0);
		}

		if(shutdown(sock, 2) < 0)
		{
			p_log_errorno((char*)"ERROR on shutdown(sock)");
			//exit(0);
		}//close(sock);
	}

	free(offsetx_history);
	free(offsety_history);
	free(inputBuffer);
}

//
// ------------------------------------ END receive_data.c------------------------------------------
//

