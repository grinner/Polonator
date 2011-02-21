// =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Release 1.0 -- 02-12-2008
// Release 2.0 -- 01-13-2009 [GP] Switch to TDI exposed memory bug (corrputed
//                                stack -- fixed by using dynamic memory allocation
//                                for arrays
// Release 2.1 -- 05-19-2009 [JE] Reset history after imaging each column.
//                                Fix for discontinuous scanning.
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
//
#include <stdio.h>
#include <stdlib.h>
#include "ProcessorParams.h"
#include "Polonator_logger.h"
#include "processor.h"


// RETURN 1 ON SUCCESS,
// -1 ON MISSING IMAGE
int ProcessImage(short unsigned int *reg_pix_list_xcols,
		 short unsigned int *reg_pix_list_yrows,
		 FILE *full_pixel_list, 
		 short unsigned int *image,
		 short unsigned int *image_info,
		 short unsigned int *beadvals,
		 FILE *beadfile,
		 FILE *sumfile,
		 FILE *imgsumfile,
		 FILE *reglogfile,
		 int *offsetx_history, 
		 int *offsety_history)
{

	// IMAGE INFO VARIABLES

	int curr_imgnum;
	int curr_arraynum;
	int curr_fcnum;


	// REGISTRATION VARIABLES
	int reg_pointer_offset;
	int offset_xcols;
	int offset_yrows;
	int score;

	// BEAD VARIABLES
	short unsigned int num_beads; // number of beads in current frame
	int i;

	double *temp_img; //gets sorted to find median pixel val  
	short unsigned int *temp_img2; //bg subtracted image passed to register
	int percentile_val;
	int temp_val;

	unsigned long long int beadsum;
	unsigned long long int imgsum;
	int index;

	char log_string[255];

	// ALLOCATE MEMORY
	if((temp_img = (double*)malloc((NUM_XCOLS*NUM_YROWS) * sizeof(double))) == NULL)
	{
		p_log_errorno((char*)"ERROR:\tProcessImage memory allocation failed (double *temp_img)");
		exit(1);
	}
	if((temp_img2 = (short unsigned int*)malloc((NUM_XCOLS*NUM_YROWS) * sizeof(short unsigned int))) == NULL)
	{
		p_log_errorno((char*)"ERROR:\tProcessImage memory allocation failed (short unsigned int *temp_img2)");
		exit(1);
	}

	// PARSE IMAGE INFO FIELDS
	curr_imgnum = (int)(*(image_info + 2));
	curr_arraynum = (int)(*(image_info + 1));
	curr_fcnum = (int)(*(image_info + 0));

	// ERROR WITH CURRENT IMAGE; PROBABLY FAILURE TO ACQUIRE
	if(*(image_info + 3) == 1)
	{
		sprintf(log_string, "ERROR:\tProcessImage: image header indicates problem with image %d %d %d",
		curr_fcnum, curr_arraynum, curr_imgnum);
		p_log(log_string);
	}

	// UPDATE POINTER TO CURRENT POSITION IN REG_PIXEL_LIST
	reg_pointer_offset = ((curr_fcnum * ARRAYS_PER_FC * IMGS_PER_ARRAY) + 
						(curr_arraynum * IMGS_PER_ARRAY) + 
						curr_imgnum);
	reg_pointer_offset = reg_pointer_offset * REG_PIXELS;
  
	sprintf(log_string, "STATUS:\tProcessImage: started operating on image %d, array %d, fc %d", 
			curr_imgnum,
			curr_arraynum,
			curr_fcnum);
	p_log(log_string);
    

#ifdef DEBUG1
	p_log((char*)"STATUS:\tProcessImage: calling ProcessImage_register");
	sprintf(log_string, "STATUS:\tProcessImage: seek to position %ld", reg_pointer_offset);
	p_log(log_string);
#endif


	// BACKGROUND SUBTRACT IMAGE FOR REGISTRATION
	index=0;
	for(i=0; i < 1000000; i+=135)
	{
		temp_img[index] = (double)*(image + i);
		index++;
	}
	quickSort(temp_img, index);
	percentile_val = (int)temp_img[index-(int)((double)index*0.9)];
	for(i=0; i < 1000000; i++)
	{
		temp_val = (int)*(image+i) - percentile_val;
		if(temp_val < 0)
		{
			temp_img2[i]=0;
		}
		else
		{
			temp_img2[i] = (short unsigned int)temp_val;
		}
	}

	// Added to correct offset history for discontinuous image tracking.
	if((curr_imgnum)%218==0)
	{
		p_log((char*)"STATUS:\tProcessImage: RESET HISTORY");
		for(i=0; i<OFFSET_HISTORY_LENGTH; i++)
		{
			*(offsetx_history + i) = 0;
			*(offsety_history + i) = 0;
		}
	}


	// REGISTER IMAGE WITH 'BEAD PIXELS' FROM MASK IMAGE
	ProcessImage_register(reg_pix_list_xcols,
			reg_pix_list_yrows,
			//image,
			temp_img2,
			reg_pointer_offset,
			REG_PIXELS,
			SEARCH_XCOLS,
			SEARCH_YROWS,
			&offset_xcols,
			&offset_yrows,
			&score,
			*(image_info+3),
			reglogfile,
			offsetx_history,
			offsety_history);
	//  fprintf(reglogfile, "=%d\t%d\t%d\t%d\t%d\n", curr_arraynum, curr_imgnum, offset_xcols, offset_yrows, score);

#ifdef DEBUG1
	sprintf(log_string, "STATUS:\tProcessImage: calling ProcessImage_extract w/ offsets %d %d",
		offset_xcols, offset_yrows);
	p_log(log_string);
#endif

  // PULL 'BEAD PIXELS' FROM IMAGE USING FULL MASK ON DISK
	ProcessImage_extract(image,
		       offset_xcols,
		       offset_yrows,
		       *(image_info+3),
		       full_pixel_list,
		       curr_imgnum,
		       curr_arraynum,
		       curr_fcnum,
		       &num_beads,
		       beadvals,
		       &beadsum);
	

#ifdef DEBUG1
	p_log((char*)"STATUS:\tProcessImage: writing output to files");
#endif

	if(fwrite(beadvals, sizeof(short unsigned int), num_beads, beadfile) < num_beads)
	{
		p_log_errorno((char*)"ERROR:\tProcessImage: write to bead file failed");
		exit(1);
	}
	if(fwrite(&beadsum, sizeof(long long int), 1, sumfile) < 1)
	{
		p_log_errorno((char*)"ERROR:\tProcessImage: write to sum file failed");
		exit(1);
	}
	imgsum = 0;
	for(i=0; i < 100000; i+=100)
	{
		imgsum+=*(image+i);
	}
	if(fwrite(&imgsum, sizeof(long long int), 1, imgsumfile) < 1)
	{
		p_log_errorno((char*)"ERROR:\tProcessImage: write to imgsum file failed");
		exit(1);
	}
  
	free(temp_img);
	free(temp_img2);

#ifdef DEBUG1
	p_log((char*)"STATUS:\tProcessImage: finished image");
#endif
	       
}


void quickSort(double numbers[], int array_size)
{
	qsort(numbers, array_size, sizeof(double), cmpNum);
}


int cmpNum(const void *p1, const void *p2)
{
	if( *(double*)p1 < *(double*)p2 )
	{
		return -1;
	}
	else if(*(double*)p1 > *(double*)p2)
	{
		return 1;
	}
	return 0;
}
