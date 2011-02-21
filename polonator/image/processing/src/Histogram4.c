#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>
#include "ProcessorParams.h"

#define DISPLAY_BINS 32
#define DISPLAY_BINWIDTH 512
#define NUM_BINS 16383

int main(int argc, char *argv[])
{

	char *info_fn;
	char *bead_fn_base;
	char bead_fn[255];
	int fcnum;
	int arraynum;

	int i, j;

	short unsigned int curr_val[4]; // holds data from info file for current image
	short unsigned int *beadvals1;
	short unsigned int *beadvals2;
	short unsigned int *beadvals3;
	short unsigned int *beadvals4;
	FILE *beadfile1;
	FILE *beadfile2;
	FILE *beadfile3;
	FILE *beadfile4;
	FILE *infofile;
	int curr_numobjs;
	int curr_bin;

	int *bins;
	int bin_max;
	int display_bin[DISPLAY_BINS];
	int num_segments;
	int threshold_count;
	int total_count;
	int above_threshold_count;

	if( (argc != 5) && (argc != 6) )
	{
		fprintf(stderr, "ERROR: must be called with 4 args: name of .info file, name of .beads file, flowcell, and array\n");
		exit(1);
	}

	info_fn = argv[1];
	bead_fn_base = argv[2];
	fcnum = atoi(argv[3]);
	arraynum = atoi(argv[4]);
	if(argc==6)
	{
		threshold_count = atoi(argv[5]);
	}
	else
	{
		threshold_count = 0;
	}
  
	if( (beadvals1=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		fprintf(stdout, "ERROR allocating memory for beadvals1:\n");
		perror(NULL);
		exit(1);
	}
	if((beadvals2=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		fprintf(stdout, "ERROR allocating memory for beadvals2:\n");
		perror(NULL);
		exit(1);
	}
	if((beadvals3=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		fprintf(stdout, "ERROR allocating memory for beadvals3:\n");
		perror(NULL);
		exit(1);
	}
	if((beadvals4=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int)))==NULL)
	{
		fprintf(stdout, "ERROR allocating memory for beadvals4:\n");
		perror(NULL);
		exit(1);
	}
    

	if((infofile = fopen(info_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening info file %s: ", info_fn);
		perror(NULL);
		exit(1);
	}

	sprintf(bead_fn, "%s_A.beads", bead_fn_base);
	if( (beadfile1 = fopen(bead_fn, "r")) == NULL )
	{
		fprintf(stderr, "ERROR opening info file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "%s_C.beads", bead_fn_base);
	if( (beadfile2 = fopen(bead_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening info file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "%s_G.beads", bead_fn_base);
	if( (beadfile3 = fopen(bead_fn, "r")) == NULL )
	{
		fprintf(stderr, "ERROR opening info file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "%s_T.beads", bead_fn_base);
	if( (beadfile4 = fopen(bead_fn, "r")) == NULL) {
		fprintf(stderr, "ERROR opening info file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}


	if( (bins=(int*)malloc(NUM_BINS * sizeof(int))) == NULL)
	{
		fprintf(stderr, "ERROR allocating memory for histogram bins: ");
		perror(NULL);
	}

	for( i=0; i < NUM_BINS; i++)
	{
		*(bins+i) = 0;
	}

	// SEEK TO STARTING POINT IN BEAD FILE
	fprintf(stdout, "Seeking to array %02d\n", arraynum);
	fflush(stdout);

	for(i=0; i < (fcnum * ARRAYS_PER_FC) + arraynum; i++)
	{
		for(j=0; j < IMGS_PER_ARRAY; j++)
		{

			// how many beads in the current image?
			if(fread(curr_val, sizeof(short unsigned int), 4, infofile) < 4)
			{
				fprintf(stderr, "ERROR: read info file at %d failed: ", i);
				perror(NULL);
				exit(1);
			}
			curr_numobjs = *(curr_val+3);

			// advance file pointer in all 4 files
			if(fseek(beadfile1, curr_numobjs * sizeof(short unsigned int), SEEK_CUR))
			{
				fprintf(stderr, "ERROR seeking in bead file1 at position %d: ", i);
				perror(NULL);
				exit(1);
			}
			if(fseek(beadfile2, curr_numobjs * sizeof(short unsigned int), SEEK_CUR))
			{
				fprintf(stderr, "ERROR seeking in bead file2 at position %d: ", i);
				perror(NULL);
				exit(1);
			}
			if(fseek(beadfile3, curr_numobjs * sizeof(short unsigned int), SEEK_CUR))
			{
				fprintf(stderr, "ERROR seeking in bead file3 at position %d: ", i);
				perror(NULL);
				exit(1);
			}
			if(fseek(beadfile4, curr_numobjs * sizeof(short unsigned int), SEEK_CUR))
			{
				fprintf(stderr, "ERROR seeking in bead file4 at position %d: ", i);
				perror(NULL);
				exit(1);
			}
		}
	}

	fprintf(stdout, "\n");
	fprintf(stdout, "HISTOGRAM FOR ARRAY %02d:\n", arraynum);

	// FILL HISTOGRAM ARRAY
	above_threshold_count = 0;
	total_count = 0;
	for(i=0; i < IMGS_PER_ARRAY; i++)
	{

		// SET CURR_NUMOBJS
		if( fread(curr_val, sizeof(short unsigned int), 4, infofile) < 4)
		{
			fprintf(stderr, "ERROR: read info file at %d failed: ", i);
			perror(NULL);
			exit(1);
		}
		curr_numobjs = *(curr_val+3);

		// LOAD BEAD DATA
		if( fread(beadvals1, 2, curr_numobjs, beadfile1) < curr_numobjs)
		{
			fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
			perror(NULL);
		}
		if( fread(beadvals2, 2, curr_numobjs, beadfile2) < curr_numobjs)
		{
			fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
			perror(NULL);
		}
		if( fread(beadvals3, 2, curr_numobjs, beadfile3) < curr_numobjs)
		{
			fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
			perror(NULL);
		}
		if( fread(beadvals4, 2, curr_numobjs, beadfile4) < curr_numobjs)
		{
			fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
			perror(NULL);
		}


		// INCREMENT BIN COUNTER
		for( j=0; j < curr_numobjs; j++)
		{
			curr_bin = (int)((double)(*(beadvals1 + j) +
						*(beadvals2 + j) + *(beadvals3 + j) + 
						*(beadvals4 + j)) / 4);

			(*(bins + curr_bin))++;

			if(curr_bin > threshold_count)
			{
				above_threshold_count++;
			}
			total_count++;
		} 
	}
	fclose(infofile);
	fclose(beadfile1);
	fclose(beadfile2);
	fclose(beadfile3);
	fclose(beadfile4);

  
	// OUTPUT DATA
	for(i=0; i < DISPLAY_BINS; i++)
	{
		display_bin[i]=0;
	}

	for(i=0; i < NUM_BINS; i++)
	{
		display_bin[(int)floor(i/DISPLAY_BINWIDTH)]+=(*(bins+i));
	}

	bin_max = 0;
	for(i=0; i < DISPLAY_BINS; i++)
	{
		if(display_bin[i] > bin_max) bin_max = display_bin[i];
	}

	for(i=0; i < DISPLAY_BINS; i++)
	{
		num_segments = (int)ceil(((double)display_bin[i]/double(bin_max)) * 80);

		fprintf(stdout, "%05d-%05d |", i * DISPLAY_BINWIDTH, ((i+1) * DISPLAY_BINWIDTH) - 1);
		for(j=0; j < num_segments; j++)
		{
			fprintf(stdout, "=");
		}
		fprintf(stdout, "\n");
	}
	setlocale(LC_NUMERIC, "en_US.iso88591");
	fprintf(stdout, "TOTAL %'d BEADS", total_count);
	if(threshold_count > 0)
	{
		fprintf(stdout, "; %'d (%d%) ABOVE THRESHOLD OF %d", above_threshold_count, (int)ceil(((double)above_threshold_count/(double)total_count)*100), threshold_count);
	}
	fprintf(stdout, "\neach '=' is approximately %'d beads\n", bin_max / 80);

	free(beadvals1);
	free(beadvals2);
	free(beadvals3);
	free(beadvals4);
}
