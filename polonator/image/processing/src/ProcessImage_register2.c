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
#include "Polonator_logger.h"
#include "ProcessorParams.h"

static void quickSort(short unsigned int[], int);
static int cmpNum(const void *, const void *);

void ProcessImage_register2(short unsigned int *pix_list_xcols,
			    short unsigned int *pix_list_yrows,
			    short unsigned int *image,
			    int num_objs,
			    int search_xcols,
			    int search_yrows,
			    int reg_pixels,
			    int *offset_xcols,
			    int *offset_yrows,
			    int *max_score,
			    short unsigned int img_error,
			    FILE *reglogfile,
			    short unsigned int *mask_image,
			    short unsigned int *reg_pix_xcols,
			    short unsigned int *reg_pix_yrows,
			    short unsigned int *sorted_pixels){

  int i, j, k,l;
  int curr_score;
  int curr_pixel;

  //short unsigned int mask_image[1000][1000]; // bitmap containing the object map for the current fl image
  //short unsigned int sorted_pixels[1000000]; // holds a sorted list of a subset of pixels from the fl image
  //short unsigned int reg_pix_xcols[1000000]; // holds the pixel coordinates for fl pixels used during the registration
  //short unsigned int reg_pix_yrows[1000000]; // sized at 1e6 bc this is the maximum number of pixels we'll use
  int index;
  int index2;
  int index3;
  short unsigned int pixel_intensity_threshold;
  double threshold_percentile;
  int threshold_index;
  int num_skipped;

  char log_string[255];

  FILE *test;
  int x,x2;

  *offset_xcols=0;
  *offset_yrows=0;
  *max_score=0;
  

  // Generate 'mask image' from bead pixel coordinates  
  index = 0;
  for(i=0; i<1000; i++){ //XCOLS
    for(j=0; j<1000; j++){ //YROWS
      *(mask_image + index) = 0;
      index++;
    }
  }
  for(i=0; i<num_objs; i++){
    *(mask_image + (*(pix_list_yrows+i)*1000) + (*(pix_list_xcols+i))) = 1;
  } 
  
  
  // Now determine what the fluorescence pixel threshold should be; we know we want
  // approximately reg_pixels pixels to be used, so sort the image and find the
  // pixel value threshold to yield that many pixels
  //
  // First, sort some pixels in the image; don't do them all because it takes too long
  index=0;
  for(i=0; i<1000000; i+=100){//+=100
    *(sorted_pixels+index) = *(image + i);
    index++;
  }
  quickSort(sorted_pixels, index);

  // Now, pick the threshold from the sorted pixel intensity list
  threshold_percentile = (double)reg_pixels / (double)1000000;
  threshold_index = index - (int)(threshold_percentile * (double)index); 
  pixel_intensity_threshold = sorted_pixels[threshold_index];
  
  // Now, build a list of pixel locations in the fluorescence image to use during the registration
  index2 = 0; // current pixel number for above threshold pixels
  index3 = 0; // current pixel number in the original image
  num_skipped = 0; // number of pixels skipped because of proximity to an edge
  for(i=0; i<1000; i++){ // for XCOLS
    for(j=0; j<1000; j++){ // for YROWS
      if(*(image + index3) >= pixel_intensity_threshold){ // add this pixel location to the list if it's above threshold
	//first, is it near an edge?  if so, don't use it
	if((i<=SEARCH_XCOLS+1)||
	   (j<=SEARCH_YROWS+1)||
	   ((NUM_XCOLS-i)<=(SEARCH_XCOLS+1))||
	   ((NUM_YROWS-j)<=(SEARCH_YROWS+1))){
	  num_skipped++;
	}
	else{
	  reg_pix_xcols[index2] = i;
	  reg_pix_yrows[index2] = j;
	  index2++;
	}
      }
      index3++;
      //index3+=2;
    }
    //    index3+=1000;
  }
  // index2 now holds the total number of pixels to be used during the registration
  reg_pixels = index2;
  fprintf(reglogfile, "%f percentile, %d index, %d threshold, %d above threshold, %d skipped, %d index3\n", threshold_percentile, threshold_index, pixel_intensity_threshold, reg_pixels, num_skipped, index3);
  
  // Now, iterate over the window specified
  *max_score = 0;
  curr_score = 0;
  for(i=(-1*(search_yrows)); i<search_yrows+1; i++){
    for(j=(-1*(search_xcols)); j<search_xcols+1; j++){
      
      for(k=0; k<reg_pixels; k++){
	curr_score += *(mask_image + ((*(reg_pix_xcols + k) + j)*1000) + (*(reg_pix_yrows + k) + i));
      }
      
      if(curr_score >= *max_score){
	*max_score = curr_score;
	*offset_xcols = j;
	*offset_yrows = i;
      }
      fprintf(reglogfile, "%d\t%d\t%d\n", j, i, curr_score); 
      curr_score = 0;
    }
  }


#ifdef DEBUG0
  fprintf(reglogfile, "%d\t%d\t%d\n", *offset_xcols, *offset_yrows, *max_score);
#endif  


#ifdef DEBUG0
  sprintf(log_string, "STATUS:\tProcessImage_register: found offset %d %d w/ score of %d",
	  *offset_xcols,
	  *offset_yrows,
	  *max_score);
  p_log(log_string);
  #endif
}


static void quickSort(short unsigned int numbers[], int array_size)
{
  qsort(numbers, array_size, sizeof(short unsigned int), cmpNum);
}


static int cmpNum(const void *p1, const void *p2){
  if(*(short unsigned int*)p1 < *(short unsigned int*)p2){
    return -1;
  }
  else if(*(short unsigned int*)p1 > *(short unsigned int*)p2){
    return 1;
  }
  return 0;
}
