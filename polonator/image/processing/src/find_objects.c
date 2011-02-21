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
/***************************************************************************************
 * find_objects.c                                                                      *
 *                                                                                     *
 * Takes a pointer to a brightfield image as input, and pointers to arrays for Xpos,   *
 * Ypos, and num_objs_found.  Generates a list of X,Y centroids of all objects in the  *
 * image.  Algorithm to determine intensity threshold computes the stdev of 1000       *
 * stdevs of subregions in the image, then multiplies by an empirically-determined     *
 * factor (-14.5).                                                                     *  
 *                                                                                     *
 * Original 'Matlab bwlabel' equivalent code was written by Jay Shendure.              *
 *                                                                                     *
 * Written by Greg Porreca (Church Lab) 12-10-2007                                     *
 *                                                                                     *
 ***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/*#include <c++/4.3.2/debug/multimap.h>*/
#include <map>
#include <algorithm>
#include <iterator>
#include <math.h>
#include "processor.h"
#include "ProcessorParams.h"

using namespace std;

#define MAX_OBJECTS 65534 //max objects per frame -- cannot be larger than max value of data type used
#define MAX_OBJSIZE 9 //maximum size of an object in pixels; objects larger than this will not be reported

int assign(int, int);

// GLOBALS
multimap <int, int> m1;
map <int, int> m2;

void find_objects(short unsigned int *raw_image,
		  short unsigned int *num_objs,
		  short unsigned int *obj_xcol,
		  short unsigned int *obj_yrow,
		  short unsigned int *segmask_image){
  
  int i, j;
  int count, count2, count3, count4, count5;  
  int flag1, flag2, N1, N2;
  short unsigned int* IP;
  int* IP2;
  int CUTOFF;

  int img_mean, img_stdev;

  unsigned short int yrows=NUM_YROWS, xcols=NUM_XCOLS;
  map <int, int> m3;
  map <int, int> m4;
  int *obj_size;
  int *centroid_xcol;
  int *centroid_yrow;
  short unsigned int return_numobjs, return_index;
  
  IP = (short unsigned int*) raw_image;

  char log_string[255];
  


  count = 0;
  m1.clear();
  m2.clear();
  m3.clear();
  m4.clear();
  
  //ONLY NEED THIS IF WE'RE SEGMENTING PRIMER IMAGES
  //DOESNT WORK NOW THAT STDEV IS IN IMG_TOOLS.C
  /*
  img_mean = stdev(raw_image, 1);
  img_stdev = stdev(raw_image, 0);
  CUTOFF = img_mean + (3*img_stdev);
  
  #ifdef DEBUG1
  sprintf(log_string, "--- mean %d, stdev %d, CUTOFF %d", img_mean, img_stdev, CUTOFF);
  p_log(log_string);
  #endif
  */

  // START BY COMPUTING IMAGE MEAN AND STDEV
  //
  img_mean = mean(raw_image);
  img_stdev = stdev2(raw_image);

  // NOW DETERMINE PIXEL VALUE THRESHOLD TO APPLY
  // USES EMPIRICAL STDEV MULTIPLIER IN PROCESSORPARAMS
  //
  CUTOFF = (int)(FINDOBJ_STDMULT * (double)(img_stdev)) + img_mean;
#ifdef DEBUG1
  sprintf(log_string, "find_objects: mean %d, std2 %d, threshold %d", img_mean, img_stdev, CUTOFF);
  p_log(log_string);
#endif


  if( (obj_size=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
    perror(NULL);
    fprintf(stderr, "ERROR allocating memory for obj_size\n");
  }
  if( (centroid_xcol=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
    perror(NULL);
    fprintf(stderr, "ERROR allocating memory for centroid_xcol\n");
  }
  if( (centroid_yrow=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
    perror(NULL);
    fprintf(stderr, "ERROR allocating memory for centroid_yrow\n");
  }

  
  if((IP2 = (int*) malloc(1000000*sizeof(int))) == NULL){
    perror(NULL);
    fprintf(stderr, "Could not allocate enough memory for IP2.");
    exit(42);
  }
  
  count3=0;
  count2=1;  // store label count 
  

  for(count = 0; count < (yrows * xcols); count++){ // 1-D index into image
    //Initialize centroid arrays for later
    if(count<MAX_BEADS_PERFRAME){
      *(obj_size + count) = 0;
      *(centroid_xcol + count) = 0;
      *(centroid_yrow + count) = 0;
    }
    
    IP2[count] = 0; // set object image value to 0
    //BEADS ARE LIGHT
    //if (IP[count] > CUTOFF) { // current pixel is above threshold	

    //BEADS ARE DARK
    if (IP[count] < CUTOFF) { // current pixel is below threshold	

      flag1 = 0; flag2 = 0;
      N1 = count - xcols;
      N2 = count - 1;
      if ((N1 >= 0) && (IP2[N1] > 0) && (IP[N1] < CUTOFF)) {flag1 = 1;} 
      if ((count % xcols != 0) && (IP2[N2]) > 0 && (IP[N2] < CUTOFF)) {flag2 = 1;}
      if (flag1 == 0 && flag2 == 0) {IP2[count] = count2; count2++;}  // untouched by labeled already-seen neighbors
      else if (flag1 == 1 && flag2 == 0) {IP2[count] = IP2[N1];}      // connected to pixel to top only
      else if (flag1 == 0 && flag2 == 1) {IP2[count] = IP2[N2];}      // connected to pixel to left only
      else {                                                          // connected to both neighbors
	IP2[count] = IP2[N1];
	if (IP2[N1] != IP2[N2]) {                                     // add entry (IP2[N1]=IP2[N2]) to equivalency table 
	  m1.insert(pair<int, int>(IP2[N1],IP2[N2]));
	  m1.insert(pair<int, int>(IP2[N2],IP2[N1]));
	  m1.insert(pair<int, int>(IP2[N1],IP2[N1]));
	  m1.insert(pair<int, int>(IP2[N2],IP2[N2]));
	}
      }
      
      /*      if (count2 > MAX_OBJECTS) {
	fprintf(stderr, "Too many objects in an image to keep track...\n");
	exit(42);
	}*/
    }
  }
    
  // consolidate equivalency list
  
  for (count4 = 1; count4 < count2; count4++) { 
    assign(count4, count4);	
  }
  
  // run through image again and reassign labels
  
  map<int,int>::iterator p;
  map<int,int>::iterator p2;
  
  count4 = 1;
  
  for (count5 = 1; count5 < count2; count5++) {
    if (m2.count(count5) == 0) {
      m2.insert(pair<int, int>(count5, -1));	
    }
    p = m2.find(count5);
    if (m3.count(p->second) > 0 && p->second != -1) {
      p2 = m3.find(p->second);
      m4.insert(pair<int,int>(count5,p2->second));
    } else {
      m4.insert(pair<int,int>(count5,count4));
      m3.insert(pair<int,int>(p->second,count4));
      count4++;
    }
  }
  
  count5 = 0;
  
  for (count4 = 1; count4 < count2; count4++) {
    p = m4.find(count4);
    if (p->second > count5) {count5 = p->second;} 
  }
    
  // Iterate over the bwlabel image, replacing values with object numbers specified by the m4 map
  // Iterate over bwlabel image, computing centroids for each object
  for(count = 0; count < (yrows * xcols); count++){
    if (IP2[count]>0) {
      p = m4.find(IP2[count]);
      IP2[count] = p->second;
      *(obj_size + p->second) = *(obj_size + p->second) + 1;;
      *(centroid_xcol + IP2[count]) += (count % xcols) + 1;
      *(centroid_yrow + IP2[count]) += (int) floor(float(count)/xcols) + 1;
    }
    // initialize segmask_image
    *(segmask_image + count) = 0;
  }    
  if(count5 > MAX_OBJECTS){
    sprintf(log_string, "WARNING: too many objects in the image; only returning first %d", MAX_OBJECTS);
    p_log(log_string);
  }

  *num_objs = count5;
  return_index=0;
  return_numobjs=0;
  if(*num_objs > MAX_OBJECTS) *num_objs = MAX_OBJECTS;
  for(i=1; i<*num_objs; i++){
    *(obj_xcol + return_index) = (int)ceil(*(centroid_xcol + i) / *(obj_size + i)); // assign return args
    *(obj_yrow + return_index) = (int)ceil(*(centroid_yrow + i) / *(obj_size + i));

    //NOW HANDLED BY processor.c ON REGFILE LOAD TO SUPPORT CHANGING WINDOW ON FLY
    /*
    if( (*(obj_xcol+return_index)<=(SEARCH_XCOLS+1))||
	(*(obj_yrow+return_index)<=(SEARCH_YROWS+1))||
	((NUM_XCOLS - *(obj_xcol+return_index)) <= (SEARCH_XCOLS+1))||
	((NUM_YROWS - *(obj_yrow+return_index)) <= (SEARCH_YROWS+1))){
      //one of the centroids is within the registration search window,
      //which is not allowed
      return_index--;
      return_numobjs--;
    }
    else if(*(obj_size+i)>9){
      return_index--;
      return_numobjs--;
    }
    */
    // DONT REPORT THIS OBJECT IF IT IS TOO BIG
    if(*(obj_size+i)>MAX_OBJSIZE){
      return_index--;
      return_numobjs--;
    }
    else{
      *(segmask_image + (*(obj_yrow + return_index) * 1000) + (*(obj_xcol + return_index))) = 1;
    }


    return_index++;
    return_numobjs++;

  }
  *num_objs = return_numobjs;

  sprintf(log_string, "   %d objects found.", count5);
  p_log(log_string);

  //free memory
  free(IP2);
  free(obj_size);
  free(centroid_xcol);
  free(centroid_yrow);
}



int assign(int A, int B) {

  multimap<int,int>::iterator iter;
  multimap<int,int>::iterator lower;
  multimap<int,int>::iterator upper;

  lower = m1.lower_bound(A);
  upper = m1.upper_bound(A);

  for (iter = lower; iter != upper; iter++) {

    if (m2.count(iter->second) == 0) {
      m2.insert(pair<int, int>(iter->second,B));
      assign(iter->second,B);
    }
  }

  return 0;
}

