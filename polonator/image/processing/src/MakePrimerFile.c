//DUAL-FLOWCELL READY BUT NOT TESTED

#include <stdio.h>
#include <stdlib.h>
#include "ProcessorParams.h"

int main(int argc, char *argv[])
{

	char *info_fn;
	char *bead_fn_base;
	char bead_fn[255];
	char output_fn[255];

	int fcnum;
	int i, j, k, m;

	short unsigned int curr_val[4]; // holds data from info file for current image
	short unsigned int *beadvals1;
	short unsigned int *beadvals2;
	short unsigned int *beadvals3;
	short unsigned int *beadvals4;
	short unsigned int *outputvals;

	FILE *beadfile1;
	FILE *beadfile2;
	FILE *beadfile3;
	FILE *beadfile4;
	FILE *outputfile;
	FILE *infofile;
	int curr_numobjs;

	if(argc != 4)
	{
		fprintf(stderr, "ERROR: must be called with 3 args: name of .info file, flowcell number, and name of .beads file\n");
		exit(1);
	}

	info_fn = argv[1];
	fcnum = atoi(argv[2]);
	bead_fn_base = argv[3];

	if((beadvals1=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
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
	if((beadvals4=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int))) == NULL)
	{
		fprintf(stdout, "ERROR allocating memory for beadvals4:\n");
		perror(NULL);
		exit(1);
	}
	if((outputvals=(short unsigned int*)malloc(MAX_BEADS_PERFRAME * sizeof(short unsigned int)))==NULL)
	{
		fprintf(stdout, "ERROR allocating memory for outputvals:\n");
		perror(NULL);
		exit(1);
	}
    

	if((infofile = fopen(info_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening info file %s: ", info_fn);
		perror(NULL);
		exit(1);
	}

	sprintf(bead_fn, "beads/%d_%s_A.beads", fcnum, bead_fn_base);
	if((beadfile1 = fopen(bead_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening bead1 file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "beads/%d_%s_C.beads", fcnum, bead_fn_base);
	if((beadfile2 = fopen(bead_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening bead2 file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "beads/%d_%s_G.beads", fcnum, bead_fn_base);
	if((beadfile3 = fopen(bead_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening bead3 file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}
	sprintf(bead_fn, "beads/%d_%s_T.beads", fcnum, bead_fn_base);
	if((beadfile4 = fopen(bead_fn, "r")) == NULL)
	{
		fprintf(stderr, "ERROR opening bead4 file %s: ", bead_fn);
		perror(NULL);
		exit(1);
	}

	// the control gui needs this file to be present to 'see' the bead file
	sprintf(output_fn, "beads/%d_PRIMER.beadsums_full", fcnum);
	if((outputfile = fopen(output_fn, "w"))==NULL)
	{
		fprintf(stderr, "ERROR opening output file %s: ", output_fn);
		perror(NULL);
		exit(1);
	}
	fclose(outputfile);
  
	sprintf(output_fn, "beads/%d_PRIMER.beads", fcnum);
	if((outputfile = fopen(output_fn, "w"))==NULL)
	{
		fprintf(stderr, "ERROR opening output file %s: ", output_fn);
		perror(NULL);
		exit(1);
	}


	for(i=0; i < FCs_PER_RUN; i++)
	{
		for(j=0; j < ARRAYS_PER_FC; j++)
		{
			for(k=0; k < IMGS_PER_ARRAY; k++)
			{

				fprintf(stdout, "Working with image %d:%d:%04d\r", i, j, k);
				fflush(stdout);

				// how many beads in the current image?
				if(fread(curr_val, sizeof(short unsigned int), 4, infofile)<4){
					fprintf(stderr, "ERROR: read info file at %d failed: ", i);
					perror(NULL);
					exit(1);
				}
				curr_numobjs = *(curr_val+3);

				if(*curr_val == fcnum)
				{
					// load input values for current image	
					if(fread(beadvals1, 2, curr_numobjs, beadfile1) < curr_numobjs)
					{
						fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
						perror(NULL);
					}
					if(fread(beadvals2, 2, curr_numobjs, beadfile2) < curr_numobjs)
					{
						fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
						perror(NULL);
					}
					if(fread(beadvals3, 2, curr_numobjs, beadfile3) < curr_numobjs)
					{
						fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
						perror(NULL);
					}
					if(fread(beadvals4, 2, curr_numobjs, beadfile4) < curr_numobjs){
						fprintf(stderr, "ERROR reading bead values for current image %d: ", i);
						perror(NULL);
					}

					// calculate output values for current image
					for(m = 0; m < curr_numobjs; m++)
					{
						*(outputvals + m) = (short unsigned int)((double)(*(beadvals1 + m) + *(beadvals2 + m) + *(beadvals3 + m) + *(beadvals4 + m)) / 4);
					}
			
					// dump results to output file
					if((fwrite(outputvals, sizeof(short unsigned int), curr_numobjs, outputfile)) < curr_numobjs)
					{
						fprintf(stderr, "ERROR writing bead values for current image %d,%d,%d: ", i, j, k);
						perror(NULL);
						exit(1);
					}
				}// end if(*curr_val==fcnum)
			} // end for k<IMGS_PER_ARRAY
		}  // end for j<ARRAYS_PER_FC
	} // end for i<FCs_PER_ARRAY

	fprintf(stdout, "\n");

	fclose(infofile);
	fclose(beadfile1);
	fclose(beadfile2);
	fclose(beadfile3);
	fclose(beadfile4);
	fclose(outputfile);

	free(beadvals1);
	free(beadvals2);
	free(beadvals3);
	free(beadvals4);
	free(outputvals);
}

