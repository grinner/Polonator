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
#include "Polonator_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
void ReceiveFilename(char*, 
		     int, 
		     char*);
int ReceiveFCNum(char*, 
		  int);
void ReceiveData(char*, 
		 int, 
		 short unsigned int*,
		 short unsigned int*,
		 FILE*,
		 short unsigned int*,
		 FILE*,
		 FILE*,
		 FILE*,
		 FILE*,
		 int);

int ProcessImage(short unsigned int*,
		 short unsigned int*,
		 FILE*,
		 short unsigned int*,
		 short unsigned int*,
		 short unsigned int*,
		 FILE*,
		 FILE*,
		 FILE*,
		 FILE*,
		 int*,
		 int*);

void find_objects(short unsigned int*,
		  short unsigned int*,
		  short unsigned int*,
		  short unsigned int*,
		  short unsigned int*);

void flatten_image(short unsigned int*,
		   short unsigned int*,
		   int);

int stdev(short unsigned int*);
int stdev2(short unsigned int*);

int mean(short unsigned int*);

void ProcessImage_extract(short unsigned int*,
			  int,
			  int,
			  short unsigned int,
			  FILE*,
			  int,
			  int,
			  int,
			  short unsigned int*,
			  short unsigned int*,
			  unsigned long long int*);

void ProcessImage_register(short unsigned int*,
			   short unsigned int*,
			   short unsigned int*,
			   int,
			   int,
			   int,
			   int,
			   int*,
			   int*,
			   int*,
			   short unsigned int,
			   FILE*,
			   int*,
			   int*);
int GetSock(char*,
	    int);

void ReceiveInitData(char*, int, FILE*, FILE*, int);


void NormalizeBeads(short unsigned int **, 
		    double **, 
		    char **,
		    double **,
		    bool **,
		    short unsigned int,
		    int,
		    int);
void FilterSumQuality(short unsigned int **,
		      double **,
		      double,
		      short unsigned int,
		      int,
		      bool*);

void quickSort(double numbers[], int array_size);
int cmpNum(const void*, const void*);
double kth_smallest(double[], int,int);
double percentile_val(double[], int, int);
double quickSelect(double[], int);
