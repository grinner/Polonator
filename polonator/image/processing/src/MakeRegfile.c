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
#include <math.h>
#include <iostream>
#include <unistd.h>
#include "ProcessorParams.h"
#include "processor.h"
#include "Polonator_logger.h"

short unsigned int randdist(short unsigned int min, short unsigned int max);

int main(int argc, char *argv[])
{
	FILE *regfile;
	FILE *posfile;
	FILE *logfile;
	FILE *dualfcoffsetfile;

	fpos_t offsetval;

	char regfilename[200];
	char logfilename[200];
	char log_string[255];

	int i, j, k, l;
	int curr_fc, curr_array, curr_img, curr_numobjs;
	int num_fcs;

	short unsigned int *curr_img_x;
	short unsigned int *curr_img_y;
	short unsigned int *regindices;
	short unsigned int temp;

	srand((unsigned)time(NULL) );
	srandom((unsigned)time(NULL));

	start_logger((char*)"initialize_processor-log", 1);
 
	if(argc !=3 )
	{
		sprintf(log_string, "Usage: %s <Position file> <Number of flowcells>", argv[0]);
		p_log(log_string);
		exit(0);
	}

	num_fcs = atoi(argv[2]);
	if( (dualfcoffsetfile = fopen("DUAL_FC_REGOFFSET.dat", "wb"))==NULL)
	{
		sprintf(log_string, "ERROR opening offset file DUAL_FC_REGOFFSET.dat for output");
		p_log_errorno(log_string);
		exit(0);
	}

	if((curr_img_x = (short unsigned int*) malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		p_log_errorno((char*)"ERROR allocating memory for curr_img_x");
		exit(0);
	}
	if((curr_img_y = (short unsigned int*) malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		p_log_errorno((char*)"ERROR allocating memory for curr_img_y");
		exit(0);
	}
	if((regindices = (short unsigned int*) malloc(REG_PIXELS * sizeof(short unsigned int))) == NULL)
	{
		p_log_errorno((char*)"ERROR allocating memory for regindices");
		exit(0);
	}

	strcpy(regfilename, argv[1]);
	strcat(regfilename, ".reg");
	sprintf(log_string, "Generating .reg file %s from position file %s with %d beads per image (random)",
	regfilename,
	argv[1],
	REG_PIXELS);
	p_log(log_string);

	strcpy(logfilename, argv[1]);
	strcat(logfilename, ".reglog");
	sprintf(log_string, "Logging positions used to file %s", logfilename);
	p_log(log_string);
  
	if( (posfile=fopen(argv[1], "rb")) == NULL)
	{
		sprintf(log_string, "ERROR opening position file %s for input", argv[1]);
		p_log_errorno(log_string);
		exit(0);
	}
	if( (regfile=fopen(regfilename, "wb")) == NULL)
	{
		sprintf(log_string, "ERROR opening reg file %s for output", argv[2]);
		p_log_errorno(log_string);
		exit(0);
	}
	if( (logfile=fopen(logfilename, "wb")) == NULL)
	{
		sprintf(log_string, "ERROR opening reglog file %s for output", argv[2]);
		p_log_errorno(log_string);
		exit(0);
	}


	// loop through FC, ARRAY, IMAGE:
	for(i=0; i < num_fcs; i++)
	{
		for(j=0; j < ARRAYS_PER_FC; j++)
		{
			for(k=0; k < IMGS_PER_ARRAY; k++)
			{
				// load positions for current image
				// ------
				// each 'record' in the pos file is as follows:
				// 4 bytes:  -1
				// 4 bytes:  flowcell # ([0..num_fcs])
				// 4 bytes:  array #    ([0..ARRAYS_PER_FC])
				// 4 bytes:  image #    ([0..IMGS_PER_ARRAY])
				// 4 bytes:  # of beads
				// 2 bytes:  beadpos_xcol
				// 2 bytes:  beadpos_yrow
				// ...
				// 2 bytes:  0
				//
				// ------

				if( (fread(&curr_fc, sizeof(int), 1, posfile)) < 1){
					p_log_errorno((char*)"ERROR reading curr_fc(1) from posfile");
				}
				if( (fread(&curr_fc, sizeof(int), 1, posfile)) < 1){
					p_log_errorno((char*)"ERROR reading curr_fc(2) from posfile");
				}
				if( (fread(&curr_array, sizeof(int), 1, posfile)) < 1){
					p_log_errorno((char*)"ERROR reading curr_array from posfile");
				}
				if( (fread(&curr_img, sizeof(int), 1, posfile)) < 1){
					p_log_errorno((char*)"ERROR reading curr_img from posfile");
				}
				if( (fread(&curr_numobjs, sizeof(int), 1, posfile)) < 1){
					p_log_errorno((char*)"ERROR reading curr_numobjs from posfile");
				}

				// make sure we are where we think we are in the position file
				if( (curr_fc != i) || (curr_array != j)|| (curr_img != k))
				{
					sprintf(log_string, "ERROR: expecting %d %d %d from position file, and read %d %d %d",
									i, j, k,
									curr_fc, curr_array, curr_img);
					p_log(log_string);
					exit(0);
				}

				sprintf(log_string, "Position %d %d %d... %d objects", i, j, k, curr_numobjs);
				p_log(log_string);

				for(l=0; l < curr_numobjs; l++)
				{
					if((fread(curr_img_x+l, sizeof(short unsigned int), 1, posfile)) < 1)
					{
						sprintf(log_string, "ERROR reading x from posfile at %d %d %d %d",
									i, j, k, l);
						p_log_errorno(log_string);
					}
					if((fread(curr_img_y+l, sizeof(short unsigned int), 1, posfile)) < 1)
					{
						sprintf(log_string, "ERROR reading y from posfile at %d %d %d %d",
								i, j, k, l);
						p_log_errorno(log_string);
					}
				}

				// make sure we are where we think we are in the position file
				if( (fread(&temp, sizeof(short unsigned int), 1, posfile)) < 1)
				{
					p_log_errorno((char*)"ERROR reading curr_img from posfile");
				}
				if(temp != 0)
				{
					sprintf(log_string, "ERROR: expecting delimiter value of 0, read %d", temp);
					p_log(log_string);
					exit(0);
				}


				// generate list of random coords and log to reglog, 2 bytes per position
				// as we generate them, output corresponding position values to regfile
				for(l = 0; l < REG_PIXELS; l++)
				{
					if(curr_numobjs < 100)
					{
						*(regindices + l) = randdist(0, NUM_XCOLS-1);
					}
					else
					{
						*(regindices + l) = randdist(0, curr_numobjs-1);
					}
					if((fwrite(regindices+l, sizeof(short unsigned int), 1, logfile)) < 1)
					{
						sprintf(log_string, "ERROR writing to reglog at %d %d %d %d", i, j, k, l);
						p_log_errorno(log_string);
						exit(0);
					}
	  
					if((fwrite(curr_img_x+(*(regindices+l)), sizeof(short unsigned int), 1, regfile)) < 1)
					{
						sprintf(log_string, "ERROR writing x to regfile at %d %d %d %d: %d",
							i,j,k,l,*(regindices+l));
						p_log_errorno(log_string);
						exit(0);
					}
					if((fwrite(curr_img_y+(*(regindices+l)), sizeof(short unsigned int), 1, regfile)) < 1)
					{
						sprintf(log_string, "ERROR writing y to regfile at %d %d %d %d: %d",
							i,j,k,l,*(regindices+l));	
						p_log_errorno(log_string);
						exit(0);
					}
				}
			} // end for k
		} // end for j
		if( i == 0 )
		{
			fgetpos(regfile, &offsetval);
			fwrite(&offsetval, sizeof(fpos_t), 1, dualfcoffsetfile);
			fclose(dualfcoffsetfile);
		}
	} // end for i
	fclose(posfile);
	fclose(regfile);
}
      
short unsigned int randdist(short unsigned int min, short unsigned int max)
{
	short unsigned int return_val;
	return_val = (short unsigned int) ((double)min + ( (rand() / ((double)RAND_MAX) ) * (double)( (double)max-(double)min ) ));
	while(return_val == max)
	{
		return_val = (short unsigned int) ((double)min + ( (rand() / ((double)RAND_MAX) ) * (double)( (double)max-(double)min ) ));
	}
	return return_val;
}
