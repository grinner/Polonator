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
#include <math.h>
#include "processor.h"
#include "ProcessorParams.h"

void flatten_image(short unsigned int* orig_image,
		   short unsigned int* new_image,
		   int rescale){
  
  int pixel_mean;
  int i, j, k, m;
  short unsigned int *index, *index2, *index3;
  short unsigned int *bg_image;
  //  short unsigned int new_image[NUM_XCOLS*NUM_YROWS];
  short unsigned int bg_min, bg_max;
  short unsigned int new_min, new_max;
  short unsigned int curr_sub;
  int new_val;

  if( (bg_image=(short unsigned int *)malloc(NUM_XCOLS*NUM_YROWS*(sizeof(short unsigned int)))) == NULL){
    p_log_errorno((char*)"malloc() failed");
    exit(0);
  }

  // INITIALIZE NEW_IMAGE AND BG_IMAGE PIXELS SINCE NOT ALL WILL BE
  // ASSIGNED ELSEWHERE
  for(i=0; i<NUM_YROWS*NUM_XCOLS; i++){
    *(new_image + i) = 0;
    *(bg_image + i) = 0;
  }


  // FIRST, GENERATE 'BACKGROUND' IMAGE BY AVERAGING OVER A PIXEL WINDOW
  // OF SUFFICIENT SIZE TO OBSCURE BEADS (e.g. FLATTEN_WINDOWSIZE=5)
  // KEEP TRACK OF MEAN AND MAX PIXEL VALS HERE AS WELL
  bg_min = 16383;
  bg_max = 0;
  for(i=FLATTEN_WINDOWSIZE; i<(NUM_YROWS-FLATTEN_WINDOWSIZE+1); i+=(FLATTEN_WINDOWSIZE*2)){
    for(j=FLATTEN_WINDOWSIZE; j<(NUM_XCOLS-FLATTEN_WINDOWSIZE+1); j+=(FLATTEN_WINDOWSIZE*2)){
      
      pixel_mean=0;
      for(k=(-1*FLATTEN_WINDOWSIZE); k<(FLATTEN_WINDOWSIZE); k++){
	for(m=(-1*FLATTEN_WINDOWSIZE); m<(FLATTEN_WINDOWSIZE); m++){
	  pixel_mean+= *(orig_image + ((i + k)*NUM_XCOLS) + (j + m));
	}
      }
      pixel_mean = (int) (pixel_mean / (int)(pow((FLATTEN_WINDOWSIZE*2),2)));
      for(k=(-1*FLATTEN_WINDOWSIZE); k<(FLATTEN_WINDOWSIZE); k++){
	for(m=(-1*FLATTEN_WINDOWSIZE); m<(FLATTEN_WINDOWSIZE); m++){
	  *(bg_image + ((i + k)*NUM_XCOLS) + (j + m)) = pixel_mean;
	  if(pixel_mean < bg_min){
	    bg_min = pixel_mean;
	  }
	  if(pixel_mean > bg_max){
	    bg_max = pixel_mean;
	  }
	}
      }
    }
  }
  
  // RESCALE BG_IMAGE SO IT STARTS AT 0, SUBTRACT FROM IMAGE; COMPUTE
  // NEW IMAGE PIXEL MAX AND MIN IF RESCALE == 1
  new_min = 16383;
  new_max = 0;
  for(i=0; i<NUM_YROWS * NUM_XCOLS; i++){
    index = orig_image + i;
    index2 = bg_image + i; // background_image pointer
    index3 = new_image + i; // new_image pointer
    
    new_val = *(index2) - bg_min;
    if(new_val<0) new_val = 0;
    //    fprintf(stdout, "setting bg array to value %d from %d\n", (short unsigned int)new_val, *index2);
    *(index2) = (short unsigned int)new_val; // this is the backgound pixel value
    

    new_val = *(index) - *(index2);
    if(new_val<0) new_val = 0;
    //    fprintf(stdout, "setting new array to %d, orig array was %d\n", (short unsigned int)new_val, *index);
    *(index3) = (short unsigned int)new_val;
    if(rescale){
      if(*(index3) > new_max){
	new_max = *(index3);
      }
      if(*(index3) < new_min){
	new_min = *(index3);
      }
    }
  }
  if(new_min==16383) new_min=0;
  new_max = new_max - new_min;

  // RESCALE IF NECESSARY
  if(rescale){
    for(i=0; i<NUM_YROWS * NUM_XCOLS; i++){
      index = new_image + i;
      *(index) = *(index) - new_min;
      *(index) = (int)((double)( *(index)) * (double)(16383/(double)(new_max)));
    }
  }
  free(bg_image);
}

  

int stdev(short unsigned int *data){
  int i;
  int img_mean;
  long long int sigma=0;
  double std=0;

  img_mean = mean(data); 

  for(i=0; i<NUM_XCOLS*NUM_YROWS; i++){
    //    fprintf(stdout, "%f\n", sqrt(pow(*(data+i)-img_mean,2)));
    sigma+=(long long int)(pow((*(data+i)-img_mean),2));
  }
  
  fprintf(stdout, "%ld, %lf\n", sigma, sqrt((1/1000000)*sigma)); 
  std = sqrt((1.0/((double)(NUM_XCOLS*NUM_YROWS)-1)) * sigma);
  fprintf(stdout, "std: %d\n", (int)std);
  return (int)std;
}

int stdev2(short unsigned int *data){
  long long int std[1000];
  long long int stdsum;
  long long int imgsum;
  int img_mean;
  int std_mean;
  long long int sigma;
  double std2;

  int i,j;

  img_mean = mean(data);
  stdsum = 0;
  for(i=0; i<NUM_YROWS; i++){
    sigma = 0;
    imgsum=0;
    for(j=0; j<NUM_XCOLS; j++){
      imgsum += (long long int)(*(data+(NUM_XCOLS*i)+j));
    }
    img_mean = imgsum/NUM_XCOLS;

    for(j=0; j<NUM_XCOLS; j++){
      sigma += (long long int)(pow((*(data+(NUM_XCOLS*i)+j))-img_mean, 2));
    }
    std[i]= (long long int)(sqrt((1.0/((double)(NUM_XCOLS-1))) * sigma));
    stdsum+=std[i];
  }
  std_mean = (int)(stdsum / NUM_YROWS);

  sigma = 0;
  for(i=0; i<NUM_YROWS; i++){
    sigma += (long long int)(pow((std[i]-std_mean),2));
  }
  std2 = sqrt((1.0/((double)(NUM_YROWS)-1)) * sigma);
  return (int)std2;
}


int mean(short unsigned int *data){
  long long int sum=0;

  int i;
  int mean;

  for(i=0; i<NUM_XCOLS*NUM_YROWS; i++){
    sum+=*(data+i);
  }
  fprintf(stdout, "sum %lld, %d\n", sum, sum);
  mean = (int) ((double)sum / (double)(NUM_XCOLS*NUM_YROWS));
  return mean;
}


 

