// =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Release 1.0 -- 02-12-2008
// Release 1.1 -- 11-19-2008 GP Modified to start offsets at nonzero values
//                              to accommodate flowcell motion during imaging
//                              NOTE offset_xcols is actually offset along
//                              stage Y axis, and offset_yrows is offset along
//                              stage X axis
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Polonator_logger.h"
#include "ProcessorParams.h"

void ProcessImage_register(short unsigned int *reg_pix_list_xcols,
			   short unsigned int *reg_pix_list_yrows,
			   short unsigned int *image,
			   int reg_pixel_list_start,
			   int reg_pixels,
			   int search_xcols,
			   int search_yrows,
			   int *offset_xcols,
			   int *offset_yrows,
			   int *max_score,
			   short unsigned int img_error,
			   FILE *reglogfile,
			   int *offsetx_history,
			   int *offsety_history)
{

	int i, j, k,l;
	int curr_score;
	int offset_start_x, course_x;
	int offset_start_y, course_y;

	char log_string[255];

	*max_score = 0;
	curr_score = 0;


	// determine current starting offset from the average of previous offsets
	// in the offset history; this allows us to correct for an offset drift across
	// the flowcell and still keep our search window small
	offset_start_x = 0;
	offset_start_y = 0;
	for(i=0; i < OFFSET_HISTORY_LENGTH; i++)
	{
		offset_start_x +=*(offsetx_history + i);
		offset_start_y +=*(offsety_history + i);
	}
	offset_start_x = (int)(floor((double)offset_start_x / (double)OFFSET_HISTORY_LENGTH));
	offset_start_y = (int)(floor((double)offset_start_y / (double)OFFSET_HISTORY_LENGTH));
#ifdef DEBUG0
	sprintf(log_string, "STATUS:\tProcessImage_register: using starting offset of %d %d", offset_start_x, offset_start_y);
	p_log(log_string);
#endif


	if(img_error == 1)
	{
		p_log((char*)"WARNING:\tProcessImage_register: current image has error flag set; setting offsets in x and y to max and score to 0");
		*max_score = 0;
		*offset_xcols = search_xcols;
		*offset_yrows = search_yrows;
	}

	else
	{
		sprintf(log_string, "STATUS:\tProcessImage_register: searching offsets from %d,%d to %d,%d",
			((-1*(search_xcols))+offset_start_x),
			((-1*(search_yrows))+offset_start_y),
			search_xcols+offset_start_x,
			search_yrows+offset_start_y);
		p_log(log_string);

		for(i=((-1*(search_yrows))+offset_start_y); i < ((search_yrows)+offset_start_y+1); i+=3)
		{
			for(j=((-1*(search_xcols))+offset_start_x); j < ((search_xcols)+offset_start_x+1); j+=3)
			{
				for(k=0; k < reg_pixels; k++)
				{
					curr_score += *(image + 
					((i + *(reg_pix_list_yrows + reg_pixel_list_start + k))*NUM_XCOLS) + 
					(j + *(reg_pix_list_xcols + reg_pixel_list_start + k)));
				} // END FOR K REG_PIXELS
				//	fprintf(reglogfile, "%d\t%d\t%d\n", i, j, curr_score);
				if(curr_score >= *max_score)
				{
					*max_score = curr_score;
					*offset_xcols = j;
					course_x = j;
					*offset_yrows = i;
					course_y = i;
				}
				curr_score = 0;
			} // END FOR J SEARCH_XCOLS
		} // END FOR I SEARCH_YROWS


		for(i=((-1*(3))+course_y); i < ((3)+course_y+1); i+=1)
		{
			for(j=((-1*(3))+course_x); j < ((3)+course_x+1); j+=1)
			{
				for(k=0; k < reg_pixels; k++)
				{
					curr_score +=  *( image + 
									( (i + *(reg_pix_list_yrows + reg_pixel_list_start + k))*NUM_XCOLS) + 
									(j + *(reg_pix_list_xcols + reg_pixel_list_start + k)));
				} // END FOR K REG_PIXELS
				//	fprintf(reglogfile, "%d\t%d\t%d\n", i, j, curr_score);
				if(curr_score >= *max_score)
				{
					*max_score = curr_score;
					*offset_xcols = j;
					*offset_yrows = i;
				}
				curr_score = 0;
			} // END FOR J SEARCH_XCOLS
		} // END FOR I SEARCH_YROWS

	} // end else


#ifdef DEBUG0
	sprintf(log_string, "STATUS:\tProcessImage_register: found offset %d %d w/ score of %d",
		*offset_xcols,
		*offset_yrows,
		*max_score);
	p_log(log_string);
#endif


	// if the offset is not near a limit (failed alignment reports offsets biased to the limits)
	// add it to the history and shift the oldest offset out
	// a target of 2+ pixels from the limit is hardcoded here
	if((abs(*offset_xcols) < MAX_ADDITIONAL_OFFSET) && 
		(abs(*offset_yrows) < MAX_ADDITIONAL_OFFSET))
	{
		if((abs(abs(*offset_xcols) - (abs(search_xcols) + abs(offset_start_x))) > 6) &&
			(abs(abs(*offset_yrows) - (abs(search_yrows) + abs(offset_start_y))) > 6))
		{
			p_log((char*)"STATUS:\tProcessImage_register:  offset is not near a limit; updating  history");
			for(i=0; i < OFFSET_HISTORY_LENGTH-1; i++)
			{
				*(offsetx_history + i) = *(offsetx_history + i + 1);
				*(offsety_history + i) = *(offsety_history + i + 1);
			}
			*(offsetx_history + i) = *offset_xcols;
			*(offsety_history + i) = *offset_yrows;
		}
	}
	else
	{
		p_log((char*)"ERROR: maximum offset reached; not moving window any further");
	}

#ifdef DEBUG0
	fprintf(reglogfile, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
		*offset_xcols, *offset_yrows, 
		*max_score, offset_start_x, offset_start_y,
		((-1*(search_xcols))+offset_start_x),
		((-1*(search_yrows))+offset_start_y),
		search_xcols+offset_start_x,
		search_yrows+offset_start_y);
#endif  
}

