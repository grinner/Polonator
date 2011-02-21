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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ProcessorParams.h"
#include "processor.h"
#include "Polonator_logger.h"

#ifdef SAVE_IMAGES
char CYCLE_NAME[255];
#endif

int main(int argc, char *argv[])
{
	FILE *posfile;
	FILE *infofile;
	char log_string[255];
	#ifdef SAVE_IMAGES
	char img_fcnum[255];
	#endif

	int portnum;// = atoi(argv[2]);
	char filename[12];
	int fcnum;
	int dual_flowcell = 0;

	start_logger((char*)"initialize_processor-log", 1);

	if(argc != 2 )
	{
		sprintf(log_string, "Usage: %s <Server hostname>", argv[0]);
		p_log(log_string);
		exit(0);
	}

	portnum = PORTNUM;

	p_log((char*)"processor:\tRequesting position filename from server ...");
	strcpy(filename, "");
	ReceiveFilename(argv[1], portnum, filename);
	sprintf(log_string, "processor:\treceived filename %s", filename);
	p_log(log_string);

	fcnum = ReceiveFCNum(argv[1], portnum);
	sprintf(log_string, "processor:\treceived fcnum %d", fcnum);
	p_log(log_string);
	if(fcnum > 1)
	{
		dual_flowcell = 1;
	}

#ifdef SAVE_IMAGES
	sprintf(img_fcnum, "%d", fcnum);
	strcat(CYCLE_NAME, img_fcnum);
	strcat(CYCLE_NAME, "_");
	strcat(CYCLE_NAME, filename);
#endif

	if( (posfile = fopen(filename, "wb")) == NULL)
	{
		sprintf(log_string, "ERROR opening output pos file %s", filename);
		p_log_errorno(log_string);
		exit(0);
	}
  
	sprintf(log_string, "processor:\tRequesting data from server for file %s ", filename);
	p_log(log_string);
	strcat(filename, ".info");
	if( (infofile = fopen(filename, "wb")) == NULL)
	{
		sprintf(log_string, "ERROR opening output info file %s", filename);
		p_log_errorno(log_string);
		exit(0);
	}


  /* fcnum will be == 2 if we're doing both flowcells; in this case,
     the next set of images will be the second half of the data necessary
     to generate the object table; re-request the filename, fcnum, and get
     the second half of the brightfield images
  */
	if (fcnum > 1)
	{
		p_log((char*)"RECEIVE_INIT_DATA...");
		ReceiveInitData(argv[1], portnum, posfile, infofile, fcnum-2);
		p_log((char*)"wait before attempting to reconnect");
		sleep(2);
		p_log((char*)"RECEIVE_FILENAME...");
		ReceiveFilename(argv[1], portnum, filename);
		p_log((char*)"RECEIVE_FCNUM...");
		fcnum = ReceiveFCNum(argv[1], portnum);
		p_log((char*)"RECEIVE_INIT_DATA...");
		ReceiveInitData(argv[1], portnum, posfile, infofile, fcnum-2);
	}
	else
	{
		ReceiveInitData(argv[1], portnum, posfile, infofile, fcnum);
	}

	fclose(posfile);
	fclose(infofile);
	return dual_flowcell + 1;
}
