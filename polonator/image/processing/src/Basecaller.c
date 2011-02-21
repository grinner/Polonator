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
#include "processor.h"
#include "ProcessorParams.h"

#define NUM_IDS 6

typedef double elem_type;
#define ELEM_SWAP(a,b) { register elem_type t=(a);(a)=(b);(b)=t;}

// data coming in is from the synthesized strand, that is, the complement
// of the bead-bound template strand.  it is normally 3'-5' synthesized strand.
// so, if we take the complement here, we get the template strand 5'-3'
char base[] = "TGCA..";
char fluor[] = "wxyz";
FILE *deltafile;
bool write_to_deltafile=0;


int main(int argc, char *argv[])
{
	FILE *infofile;
	FILE *beadfile[MAX_BEADFILES];
	FILE *basecallfile[FCs_PER_RUN * ARRAYS_PER_FC];
	FILE *tetrafile[FCs_PER_RUN * ARRAYS_PER_FC];
	FILE *primer_threshold_fp;
	FILE *primer_fp;
	int i, j, k, m, n, index;
	char info_fn[255];
	char bead_fn[255];

	short unsigned int curr_val[4];
	short unsigned int curr_numobjs;
	short unsigned int *beadvals[MAX_BEADFILES];
	short unsigned int *beadvals_copy[MAX_BEADFILES];
	double *normvals[MAX_BEADFILES];
	bool *normflags[MAX_BEADFILES];
	char *assignments[MAX_BEADFILES];
	double *assign_quals[MAX_BEADFILES];

	int primer_thresholds[FCs_PER_RUN * ARRAYS_PER_FC];
	short unsigned int *primer_vals;

	long long int size;
	double percent_tokeep;
	int threshold_index;
	double threshold;
	int temp_sum;

	int bgsubtr_percent;
	
	// command line args
	int num_beadfiles;
	int do_basecalling;
	int do_tetra;
	char output_filename[255]; //base filename
	char output_bcfn[255]; //basecall filename
	char output_tfn[255]; //tetra filename
	char temp_string[255];

	if(argc < 2)
	{
		fprintf(stderr, "ERROR: %s called with 0 arguments\n", argv[0]);
		exit(0);
	}
	
	// we assume this program is called by a perl script which will feed it 
	// 'valid' parameters -- this is indicated by starting the arg list with
	// the string below; all bets are off if the perl script is not well-behaved
	// if somebody tries to run this directly, and doesn't know what they're
	// doing, just stop immediately
	if( strcmp(argv[1], "notruncmdln") != 0)
	{
		fprintf(stderr, "ERROR: %s should not be run directly from the command line\n", argv[0]);
		exit(0);
	}
	
	do_basecalling = atoi(argv[2]);
	do_tetra = atoi(argv[3]);
	strcpy(output_filename, "");
	strcat(output_filename, argv[4]);
	strcpy(info_fn, "");
	strcat(info_fn, argv[5]);
	percent_tokeep = (double)(atoi(argv[6]))/100;
	bgsubtr_percent = atoi(argv[7]);
	num_beadfiles = argc - 8;

	
	if( (!do_basecalling) && (!do_tetra) )
	{
		fprintf(stderr, "ERROR: nothing to do: %s called with flags %d %d\n", 
			argv[0], do_basecalling, do_tetra);
		exit(0);
	}
	
	fprintf(stdout, "Basecaller.c executing (%d %d %0.2f) with %d bead files; output to %s...\n",
		do_basecalling, do_tetra, percent_tokeep, num_beadfiles, output_filename);

	// open input files
	if( (infofile = fopen(info_fn, "r")) == NULL )
	{
		fprintf(stderr, "ERROR opening file %s: ", info_fn);
		perror(NULL);
		exit(0);
	}

	
	// Load primer threshold values for each lane.  This file contains one integer
	// value per line, between 0 and 16383, which is the minimum primer value
	// allowed for basecalling to be performed on a bead.  Lines correspond to lanes
	// as follows:
	// Line 1  -- Flowcell 0 Lane 0
	// Line 2  -- Flowcell 0 Lane 1
	// ..
	// Line 9  -- Flowcell 1 Lane 0
	// Line 16 -- Flowcell 1 Lane 7  
	if( (primer_threshold_fp = fopen(PRIMER_THRESHOLD_FILENAME, "r")) == NULL )
	{
		fprintf(stdout, "ERROR opening file %s: ", PRIMER_THRESHOLD_FILENAME);
		perror(NULL);
		fprintf(stdout, "\nUsing primer thresholds of 0 for all lanes\n"); 
	}

	for(i=0; i < FCs_PER_RUN * ARRAYS_PER_FC; i++)
	{
		if( primer_threshold_fp != NULL )
		{
			if( fscanf(primer_threshold_fp, "%d\n", primer_thresholds + i) == EOF )
			{
				fprintf(stdout, "ERROR: primer threshold file appears to be incomplete; reading line %d failed\n", i);
				exit(1);
			}
			fprintf(stdout, "OK, %d\n", *(primer_thresholds+i));
		}
		else
		{
			*(primer_thresholds+i)=0;
			fprintf(stdout, "NULL POINTER\n");
		}
		fprintf(stdout, "Set primer threshold for lane %d to %d\n", i, (int)*(primer_thresholds+i));
	}

	if(primer_threshold_fp != NULL)
	{
		if((primer_fp = fopen("beads/0_PRIMER.beads", "r")) == NULL)
		{
			fprintf(stdout, "ERROR opening file %s: ", "beads/0_PRIMER.beads");
			perror(NULL);
			exit(0);
		}
	}


	for( i=0; i < num_beadfiles; i++)
	{
		//    strcpy(bead_fn, "beads/");
		strcpy(bead_fn, "");
		strcat(bead_fn, argv[i+8]);
		strcat(bead_fn, ".beads");
		fprintf(stdout, "Opening file %s for input...\n", bead_fn);
		if( (*(beadfile + i) = fopen(bead_fn, "r")) == NULL )
		{
			fprintf(stderr, "ERROR opening file %s: ", bead_fn);
			perror(NULL);
			exit(0);
		}
	}
  
	// open output files
	index = 0;
	for(i=0; i < FCs_PER_RUN; i++)
	{
		for(j=0; j < ARRAYS_PER_FC; j++)
		{
			if(do_basecalling)
			{
				strcpy(output_bcfn, "output_data/");
				strcpy(temp_string, "");
				strcat(output_bcfn, output_filename);
				sprintf(temp_string, "_%01d_%02d.basecalls", i, j);
				strcat(output_bcfn, temp_string);
				fprintf(stdout, "Opening file %s for output...\n", output_bcfn);
				if( (*(basecallfile + index) = fopen(output_bcfn, "w")) == NULL )
				{
					fprintf(stderr, "ERROR opening file %s: ", output_bcfn);
					perror(NULL);
					exit(0);
				}
			
			}
			if(do_tetra)
			{
				strcpy(output_tfn, "tetrahedra/");
				strcpy(temp_string, "");
				strcat(output_tfn, output_filename);
				sprintf(temp_string, "_%01d_%02d.tetracoords", i, j);
				strcat(output_tfn, temp_string);
				fprintf(stdout, "Opening file %s for output...\n", output_tfn);
				if( (*(tetrafile + index) = fopen(output_tfn, "w")) == NULL )
				{
					fprintf(stderr, "ERROR opening file %s: ", output_tfn);
					perror(NULL);
					exit(0);
				}
			}
			index++;
		}
	}

	strcpy(output_tfn, "tetrahedra/");
	strcat(output_tfn, output_filename);
	strcat(output_tfn, ".delta");
	if( (deltafile = fopen(output_tfn, "w"))==NULL )
	{
		fprintf(stderr, "ERROR opening file %s: ", output_tfn);
		perror(NULL);
		exit(0);
	}
	
	
	//------------------ LOAD CURRENT FRAME OF DATA
	//
	// iterate over data in files
	index = 0;
	for( i=0; i < FCs_PER_RUN; i++ )
	{
		for( j=0; j < ARRAYS_PER_FC; j++ )
		{
			for( k=0; k < IMGS_PER_ARRAY; k++ )
			{
			
				fprintf(stdout, "Basecaller.c:\t%d %d %d: ", i, j, k);
				fflush(stdout);

				// read info file to determine number of objs in current frame
				if( fread(curr_val, 2, 4, infofile)<4 )
				{
					fprintf(stderr, "ERROR: read info file at %d %d %d failed: ", i, j, k);
					perror(NULL);
					exit(0);
				}
				curr_numobjs = *(curr_val+3);

				// LOAD PRIMER DATA FOR CURRENT FRAME
				//allocate memory
				if( (primer_vals=(short unsigned int*)malloc(curr_numobjs*sizeof(short unsigned int))) == NULL )
				{
					fprintf(stderr, "ERROR allocating memory for primer value array: ");
					perror(NULL);
				}

				//read data
				if( primer_threshold_fp != NULL )
				{
					if( fread(primer_vals, 2, curr_numobjs, primer_fp) < curr_numobjs )
					{
						fprintf(stderr, "ERROR loading primer file: ");
						perror(NULL);
					}
				}

				// allocate memory for current frame, then load bead data
				size = 0;
				for( m=0; m < num_beadfiles; m++ )
				{
					if( (beadvals[m]=(short unsigned int*)malloc(curr_numobjs*sizeof(short unsigned int))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array beadvals at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(short unsigned int));
						perror(NULL);
					}
					size += (curr_numobjs * sizeof(short unsigned int));
					if( (beadvals_copy[m]=(short unsigned int*)malloc(curr_numobjs*sizeof(short unsigned int))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array beadvals_copy at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(short unsigned int));
						perror(NULL);
					}
					size += (curr_numobjs * sizeof(short unsigned int));

					if( (normvals[m]=(double*)malloc(curr_numobjs*sizeof(double))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array normvals at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(double));
						perror(NULL);
					}
					size += (curr_numobjs * sizeof(double));

					if( (normflags[m]=(bool*)malloc(curr_numobjs*sizeof(bool))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array normflags at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(bool));
						perror(NULL);
					}
					size+=(curr_numobjs * sizeof(bool));

					if( (assignments[m]=(char*)malloc(curr_numobjs*sizeof(char))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array assignments at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(char));
						perror(NULL);
					}
					size += (curr_numobjs * sizeof(char));

					if( (assign_quals[m]=(double*)malloc(curr_numobjs*sizeof(double))) == NULL )
					{
						fprintf(stderr, "ERROR allocating memory for array assign_quals at %d %d %d, size %d bytes: ",
							i, j, k, curr_numobjs * sizeof(double));
						perror(NULL);
					}
					size += (curr_numobjs * sizeof(double));
					if( fread(beadvals[m], 2, curr_numobjs, *(beadfile + m)) < curr_numobjs )
					{
						fprintf(stderr, "ERROR loading beadfile %d at %d %d %d: ", m, i, j, k);
						perror(NULL);
					}

					// initialize arrays;
					// normalize all beads the first time around
					// make a copy of the raw data for use during second normalization
					// APPLY PRIMER THRESHOLD HERE TO NORMFLAGS ARRAY
					for( n=0; n < curr_numobjs; n++ )
					{
						*(normflags[m] + n) = 1; // normalize by default
						if( primer_threshold_fp != NULL )
						{
							// fprintf(stdout, "Threshold (%d,%d) is %d, primer val for bead %d in image %d is %d\n", i, j,primer_thresholds[(i*ARRAYS_PER_FC) + j], n, k,*(primer_vals + n)  );
							if( *(primer_vals + n) < primer_thresholds[(i*ARRAYS_PER_FC) + j] )
							{ // below threshold; don't normalize this bead
								*(normflags[m] + n) = 0;
								//fprintf(stdout, "below threshold\n");
							}
						}
						*(beadvals_copy[m]+n) = *(beadvals[m]+n);
					} // end for n
				} // end for m
#ifdef BCDEBUG0
				fprintf(stdout, "allocated %dK of memory... \n", size/1024);
#endif

	
				//--- NORMALIZE
				write_to_deltafile = 0;
				NormalizeBeads(beadvals, normvals, assignments, assign_quals, normflags, curr_numobjs, num_beadfiles, bgsubtr_percent);

				//------ APPLY QUALITY THRESHOLD
				FilterSumQuality(beadvals_copy, assign_quals, percent_tokeep, curr_numobjs, num_beadfiles/4, normflags[0]);

				//------ RE-NORMALIZE SUBSET
				write_to_deltafile = 1;
				fprintf(deltafile, "%d\t%d\t%d", i, j, k);
				NormalizeBeads(beadvals_copy, normvals, assignments, assign_quals, normflags, curr_numobjs, num_beadfiles, bgsubtr_percent);
	

				//------------------ OPTIONAL: GENERATE AND OUTPUT TETRA COORDS
				if(do_tetra)
				{
					//if((k%100)==0){ // CHANGED TO OUTPUT EVERY 3rd CALL FOR ALL FRAMES, INSTEAD OF EVERY CALL EVERY 100TH FRAME
					//for(m=0; m<curr_numobjs; m++){
					for( m=0; m < curr_numobjs; m+=3 )
					{
						temp_sum = 0;
						for( n=0; n < (num_beadfiles/4); n++ )
						{
							temp_sum+=(int)(!((bool)(*(normflags[n]+m))));
						}
						if(temp_sum == 0)
						{
							fprintf(*(tetrafile+index), "%d\t%d\t%d\t%d", i, j, k, m);
							for(n=0; n<(num_beadfiles/4); n++)
							{
								
								fprintf(*(tetrafile+index), "\t%d\t%.05f\t%d\t%d\t%d\t%d\t%.05f\t%.05f\t%.05f\t%.05f",
								*(assignments[n]+m),
								*(assign_quals[n]+m),
								*(beadvals[(n*4)+0]+m),
								*(beadvals[(n*4)+1]+m),
								*(beadvals[(n*4)+2]+m),
								*(beadvals[(n*4)+3]+m),
								*(normvals[(n*4)+0]+m),
								*(normvals[(n*4)+1]+m),
								*(normvals[(n*4)+2]+m),
								*(normvals[(n*4)+3]+m));
							}//end for(n)
							fprintf(*(tetrafile+index), "\n");
						}//end if(temp_sum)
					}//end for(m)
					//}//end if(k)
					fflush(*(tetrafile+index));
				}//end if(do_tetra)

				//------------------ OPTIONAL: OUTPUT BASECALLS
				if(do_basecalling)
				{
					for( m=0; m < curr_numobjs; m++ )
					{
						fprintf(*(basecallfile+index), "%d\t%d\t%d\t%d\t",i,j,k,m);

						// basecalls
						for( n=0; n < (num_beadfiles/4); n++ )
						{
							if( *(normflags[n]+m) > 0 )
							{
								fprintf(*(basecallfile+index), "%c", base[*(assignments[n]+m)]);
							}
							else
							{
								fprintf(*(basecallfile+index), "%c", base[4]);
							}
						}

						// quality scores
						for(n=0; n < (num_beadfiles/4); n++)
						{
							fprintf(*(basecallfile+index), "\t%04d", (int)(*(assign_quals[n]+m)*1000));
						}

						fprintf(*(basecallfile+index), "\n");
					}
					fflush(*(basecallfile+index));
				}

				// FREE MEMORY
				for( m=0; m < num_beadfiles; m++ )
				{
					free(beadvals[m]);
					free(beadvals_copy[m]);
					free(normvals[m]);
					free(normflags[m]);
					free(assignments[m]);
					free(assign_quals[m]);
				}
				if(primer_threshold_fp != NULL)
				{
					free(primer_vals);
				}
			}//end for k
			index++;
		}//end for j
	}//end for i
}

void FilterSumQuality(short unsigned int *beadvals[MAX_BEADFILES],
						double *assign_quals[MAX_BEADFILES],
						double percent_tokeep,
						short unsigned int curr_numobjs,
						int num_cycles,
						bool *primer_thresh_flag)
{

	int i, j;
	
	int threshold_index;
	double threshold;
	double *sum_quals;
	double *sum_quals_sorted;

	int above_thresh_objects = 0;

	if((sum_quals=(double*)malloc(curr_numobjs*sizeof(double))) == NULL)
	{
		fprintf(stderr, "ERROR allocating memory for array sum_quals size %d bytes: ",
	    curr_numobjs * sizeof(double));
		perror(NULL);
	}
	if( (sum_quals_sorted=(double*)malloc(curr_numobjs*sizeof(double)))==NULL )
	{
		fprintf(stderr, "ERROR allocating memory for array sum_quals_sorted size %d bytes: ",
			curr_numobjs * sizeof(double));
		perror(NULL);
	}

	// for each bead, compute its sum quality score (across all cycles)
	// ONLY if its primer value is above threshold (specified by primer_thresh_flag==1)
	for( i=0; i < curr_numobjs; i++ )
	{
		*(sum_quals+i)=0;
		if(*(primer_thresh_flag+i))
		{
			above_thresh_objects++;
			for(j=0; j < num_cycles; j++)
			{
				//	*(sum_quals+i)+=*(assign_quals[j]+i);
				*(sum_quals+(above_thresh_objects-1))+=*(assign_quals[j]+i);
			}
			//      *(sum_quals_sorted+i) = *(sum_quals+i);
			*(sum_quals_sorted+(above_thresh_objects-1)) = *(sum_quals+(above_thresh_objects-1));
		}
	}

	// now, sort these sums
	// data sorted lowest to highest, highest being worst quality

	// determine the sum quality threshold
	//  threshold = percentile_val(sum_quals_sorted, curr_numobjs, (int)(percent_tokeep*100));
	threshold = percentile_val(sum_quals_sorted, above_thresh_objects, (int)(percent_tokeep*100));

	// apply this threshold to the data by 'zeroing out' any elements whose
	// basecall quality sum was > threshold
	// the normalizer will then ignore these on the next pass
	above_thresh_objects=0;
	for(i=0; i < curr_numobjs; i++)
	{
		if( (!(*(primer_thresh_flag+i))) || (*(sum_quals + (above_thresh_objects++)) > threshold))
		{
			//    if(*(sum_quals+i) > threshold){
			for(j=0; j < num_cycles; j++)
			{
				*(beadvals[j*4]+i) = 0;
		}
		}
	}


#ifdef BCDEBUG0
	fprintf(stdout, "Quality threshold: %.05f\n", threshold);
#endif

	free(sum_quals);
	free(sum_quals_sorted);
}


void NormalizeBeads(short unsigned int *beadvals[MAX_BEADFILES], 
					double *normvals[MAX_BEADFILES],
					char *assignments[MAX_BEADFILES],
					double *assign_quals[MAX_BEADFILES],
					bool *normflags[MAX_BEADFILES],
					short unsigned int curr_numobjs,
					int num_beadfiles,
					int bgsubtr_percent)
{

	long long int colsums[MAX_BEADFILES];
	int i, j, k, m;
	double sumsquares;
	double *norm[MAX_BEADFILES];
	double *sortedbeadvals[MAX_BEADFILES];
	double *temp_valarray[4];  
	double curr_max;
	char curr_id;
	int id_counter[NUM_IDS][MAX_BEADFILES]; // for assignments 0-6
	double assign_median[MAX_BEADFILES][4][4];
	double temp_med_dist[4];
	int num_sets = (int)(num_beadfiles/4);
	int num_quals;
	int sub_val[MAX_BEADFILES];
	double sub_fraction;
	int num_nonzero_beads[MAX_BEADFILES];
	int index;
	double temp_sort[4];

	sub_fraction = (double)bgsubtr_percent / (double)100;
  
	for( i=0; i < num_beadfiles; i++ )
	{
		if((norm[i]=(double*)malloc(curr_numobjs*sizeof(double))) == NULL )
		{
			fprintf(stderr, "ERROR allocating memory for array norm at %d, size %d bytes: ",
				i, curr_numobjs * sizeof(double));
			perror(NULL);
		}
		if( (sortedbeadvals[i]=(double*)malloc(curr_numobjs*sizeof(double)) ) == NULL )
		{
			fprintf(stderr, "ERROR allocating memory for array sortedbeadvals at %d, size %d bytes: ",
				i, curr_numobjs * sizeof(double));
			perror(NULL);
		}
	}
	for( i=0; i < 4; i++ )
	{
		if( (temp_valarray[i]=(double*)malloc(curr_numobjs*sizeof(double))) == NULL )
		{
			fprintf(stderr, "ERROR allocating memory for array temp_valarray at %d, size %d bytes: ",
				i, curr_numobjs * sizeof(double));
			perror(NULL);
		}
	}

	// initialize arrays
	for( i=0; i < num_beadfiles; i++ )
	{
		colsums[i] = 0;
		num_nonzero_beads[i] = 0;
	}
	for( i=0; i < NUM_IDS; i++ )
	{
		for(j=0; j < MAX_BEADFILES; j++)
		{
			id_counter[i][j]=0;
		}
	}

	// iterate over bead vals and populate normflag array
	// bool *normflags is NxS, N=#of beads, S=#of sets (a
	// 'set' is 4 adjacent columns of flourescence data in the
	// beadvals matrix
	// normflag == 0 if one (or more) bead vals for the current
	// set == 0 (is 'invalid')
	for( i=0; i < num_beadfiles; i++ )
	{
		for( j=0; j < curr_numobjs; j++ )
		{
			if( *(beadvals[i] + j) ==  0 )
			{
				*(normflags[i/4] + j) = 0;
			}
		}
	}
  
	// now copy data values to sortedbeadvals so we can sort them
	// keep track of how many beads are nonzero (valid) in each column
	for( i=0; i < num_beadfiles; i++ )
	{
		index=0;
		for( j=0; j < curr_numobjs; j++ )
		{
			if(*(normflags[i/4]+j))
			{
				*(sortedbeadvals[i] + index++) = (double)*(beadvals[i]+j);
			}
		}
		num_nonzero_beads[i] = index;
	}

	// determine percentile value for background subtraction
#ifdef BCDEBUG0
	fprintf(stdout, "\nNormalizeBeads(): background subtraction vals:\n");
#endif
	for( i=0; i < num_beadfiles; i++ )
	{
		sub_val[i] = (int)percentile_val(sortedbeadvals[i], num_nonzero_beads[i], (int)(sub_fraction*100));
		//    quickSort(sortedbeadvals[i], num_nonzero_beads[i]);
		//sub_val[i] = (int)sortedbeadvals[i][(int)(floor(num_nonzero_beads[i] * sub_fraction))];
#ifdef BCDEBUG0
		fprintf(stdout, "%d\t", sub_val[i]);
#endif
	}
#ifdef BCDEBUG0
	fprintf(stdout, "\n");
#endif

	// iterate over bead vals and background subtract, then compute column-wise sums
	for( i=0; i < num_beadfiles; i++ )
	{
		for( j=0; j < curr_numobjs; j++ )
		{
			if( (int)*(beadvals[i] + j) - (int)(sub_val[i]) < 0 )
			{
				*(beadvals[i]+j) = 0;
			}
			else
			{
				*(beadvals[i]+j) -= sub_val[i];
			}
			// only add to sum if we're looking at a 'valid' bead
			// 'invalid' beads are flagged as '0' in normflags
			if( *(normflags[i/4]+j) == 1 )
			{ // don't increment if another member of the set is 0
				colsums[i] += *(beadvals[i] + j);
			}
		}
	}
  

	// make sure all column sums are > 0
#ifdef BCDEBUG0
	fprintf(stdout, "\nNormalizeBeads(): column-wise sums:\n");
#endif
	for( i=0; i < num_beadfiles; i++ )
	{
		colsums[i] += 1;
#ifdef BCDEBUG0
		fprintf(stdout, "%d\t", colsums[i]);
#endif
	}
#ifdef BCDEBUG0
	fprintf(stdout, "\n");
#endif

	// divide all elements of each column by the column sum, then compute norms
	// int *norm is NxS, N=#of beads, S=#of sets
	for( i=0; i < curr_numobjs; i++ )
	{
		for( j=0; j < num_beadfiles; j++ )
		{
			if( *(normflags[j/4] + i) == 1 )
			{
				*(normvals[j] + i) = *(beadvals[j] + i) / (double)colsums[j];
				if( (j%4)==0 )
				{
					sumsquares = 0;
				}
				sumsquares += ((*(normvals[j]+i)) * (*(normvals[j]+i)));
				if( ((j+1)%4)==0 )
				{
					*(norm[(int)(floor((j-3)/4))]+i) = sqrt(sumsquares);
				}
			}
		}
	}

	// make sure all norms are > 0
	for( i=0; i < num_beadfiles/4; i++ )
	{
		for( j=0; j < curr_numobjs; j++ )
		{
			*(norm[i] + j) += DBL_EPSILON;
		}
	}
	
	
	// convert to unit vectors
	for( i=0; i < curr_numobjs; i++ )
	{
		for( j=0; j < num_beadfiles; j++)
		{
			*(normvals[j] + i) = *(normvals[j] + i) / *(norm[j/4] + i);
		}
	}


	// make initial identity assignments; each bead, for now, will be
	// assigned to the vertex it's closest to (its max)
	for( i=0; i < curr_numobjs; i++ )
	{
		for( j=0; j < num_beadfiles; j+=4 )
		{
			if( *(normflags[j/4]+i) )
			{
				curr_max = 0;
				curr_id = 5; // 0-3 are ACGT; 4 is no data (1 or more bead vals == 0), 5 is all unit vectors equal
				for( k=0; k < 4; k++)
				{
					if( *(normvals[j+k]+i) > curr_max)
					{
						curr_max = *(normvals[j+k]+i);
						curr_id = k;
					}
				}
				*(assignments[j/4] + i) = curr_id;
			}
			else
			{
				*(assignments[j/4] + i) = 4;
			}
			id_counter[*(assignments[j/4]+i)][j/4]++; // keep track of how many times each call is made
		}
	}

#ifdef BCDEBUG0
	fprintf(stdout, "\nNormalizeBeads(): basecall counts:\n");
	fprintf(stdout, "Cycle\tA\tC\tG\tT\tN\tO\n");
	for( i=0; i < num_beadfiles/4; i++ )
	{
		fprintf(stdout, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n", i,
		id_counter[0][i],
		id_counter[1][i],
		id_counter[2][i],
		id_counter[3][i],
		id_counter[4][i],
		id_counter[5][i]);
		id_counter[0][i]=0;
		id_counter[1][i]=0;
		id_counter[2][i]=0;
		id_counter[3][i]=0;
		id_counter[4][i]=0;
		id_counter[5][i]=0;
	}
	//fprintf(stdout, "\n");
#endif

	// now, compute medians of each 'cluster', and re-assign beads to the cluster they're closest to
	//
	// first, compute 'quality' of each assignment as the Euclidean distance
	// between the bead's value in 4-space and its assigned vertex
	for( i=0; i < curr_numobjs; i++)
	{
		for( j=0; j < num_beadfiles; j+=4 )
		{
			if( *(assignments[j/4]+i) == 0 )
			{
				*(assign_quals[j/4]+i) = sqrt((pow(*(normvals[j]+i) - 1, 2))+
						(pow(*(normvals[j+1]+i), 2)) +
						(pow(*(normvals[j+2]+i), 2)) +
						(pow(*(normvals[j+3]+i), 2)));
			}
			if(*(assignments[j/4]+i) == 1)
			{
				*(assign_quals[j/4]+i) = sqrt((pow(*(normvals[j]+i), 2))+
							(pow(*(normvals[j+1]+i) - 1, 2)) +
							(pow(*(normvals[j+2]+i), 2)) +
							(pow(*(normvals[j+3]+i), 2)));
			}
			if( *(assignments[j/4]+i) == 2 )
			{
				*(assign_quals[j/4]+i) = sqrt((pow(*(normvals[j]+i), 2))+
							(pow(*(normvals[j+1]+i), 2)) +
							(pow(*(normvals[j+2]+i) - 1, 2)) +
							(pow(*(normvals[j+3]+i), 2)));
			}
			if( *(assignments[j/4]+i) == 3 )
			{
				*(assign_quals[j/4]+i) = sqrt((pow(*(normvals[j]+i), 2))+
							(pow(*(normvals[j+1]+i), 2)) +
							(pow(*(normvals[j+2]+i), 2)) +
							(pow(*(normvals[j+3]+i) - 1, 2)));
			}
			else
			{
				*(assign_quals[j/4]+i) = 1;
			}
		}
	}


  // now, sort to get the median values
	for( m=0; m < 4; m++ )
	{ // for each identity (A, C, G, T)
		for( i=0; i < num_beadfiles; i+=4 )
		{ // for each cycle

			for( j=0; j < 4; j++ )
			{ // for each w,x,y,z (fluor value)
				num_quals = 0;	
				for(k=0; k < curr_numobjs; k++ )
				{ // iterate over all assignments
					if(*(assignments[i/4]+k) == m)
					{
						// lists all normvals having assignment m for set i/4; 4-cols
						temp_valarray[j][num_quals] = *(normvals[i+j]+k);
						num_quals++;
					}
				} // end for k
				if(num_quals > 0)
				{
					//assign_median[i/4][j][m] = quickSelect(temp_valarray[j], num_quals);
					assign_median[i/4][j][m] = percentile_val(temp_valarray[j], num_quals, 50);
					//quickSort(temp_valarray[j], num_quals);
					//assign_median[i/4][j][m] = temp_valarray[j][(int)(floor(num_quals/2))];
					//	  fprintf(stdout, "median cycle %d (%c %c) %0.15lf\n", i/4, fluor[j], base[m], temp_valarray[j][(int)(floor(num_quals/2))]);
				}
				else
				{
					// fprintf(stdout, "ERROR: NormalizeBeads(): beadfiles %d-%d have 0 assignments of %d\n", i, i+3, m);
					assign_median[i/4][j][m] = 1;
					//fprintf(stdout, "median cycle %d (%c %c) %0.15lf\n", i/4, fluor[j], base[m], 1);
				}
			} // end for j
		}
	}



	// now, re-assign based on new median values
	for(i=0; i < curr_numobjs; i++)
	{
		for(j=0; j < num_beadfiles; j+=4)
		{
			if( *(normflags[j/4]+i) )
			{

				// compute distances to 4 medians (A, C, G, T)
				for( k=0; k<4; k++ )
				{ // for each 4-space median (Awxyz, Cwxyz, Gwxyz, Twxyz)
					// normvals[wxyz]+bead, assign_median[cycle][wxyz][ACGT]
					temp_med_dist[k] = sqrt((pow(*(normvals[j]+i)-assign_median[j/4][0][k],2))+
							(pow(*(normvals[j+1]+i)-assign_median[j/4][1][k],2))+
							(pow(*(normvals[j+2]+i)-assign_median[j/4][2][k],2))+
							(pow(*(normvals[j+3]+i)-assign_median[j/4][3][k],2)));
				}
				curr_max = 1;
				curr_id = 3;
				for( m=0; m < 4; m++ )
				{
					if( temp_med_dist[m] < curr_max )
					{
						curr_max = temp_med_dist[m];
						curr_id = m;
					}
				}
				*(assignments[j/4]+i) = curr_id;
			}
			id_counter[*(assignments[j/4]+i)][j/4]++;
		}
      
	}

#ifdef BCDEBUG0
	fprintf(stdout, "\nNormalizeBeads(): new basecall counts:\n");
	fprintf(stdout, "Cycle\tA\tC\tG\tT\tN\tO\n");
	for( i=0; i < num_beadfiles/4; i++ )
	{
		fprintf(stdout, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n", i,
			id_counter[0][i],
			id_counter[1][i],
			id_counter[2][i],
			id_counter[3][i],
			id_counter[4][i],
			id_counter[5][i]);
	}
	fprintf(stdout, "\n");
#endif


	// now, re-compute quality scores based on new medians
	//
	// sort to get the median values
	for( m=0; m < 4; m++ )
	{ // for each identity (A, C, G, T)
		for( i=0; i < num_beadfiles; i+=4 )
		{ // for each cycle

			for( j=0; j < 4; j++ )
			{ // for each w,x,y,z (fluor value)
				num_quals = 0;	
				for( k=0; k < curr_numobjs; k++ )
				{ // iterate over all assignments
					if( *(assignments[i/4]+k) == m )
					{
						// lists all normvals having assignment m for set i/4; 4-cols
						temp_valarray[j][num_quals] = *(normvals[i+j]+k);
						num_quals++;
					}
				} // end for k
				if( num_quals > 0 )
				{
					//assign_median[i/4][j][m] = quickSelect(temp_valarray[j], num_quals);
					assign_median[i/4][j][m] = percentile_val(temp_valarray[j], num_quals, 50);
					//quickSort(temp_valarray[j], num_quals);
					//assign_median[i/4][j][m] = temp_valarray[j][(int)(floor(num_quals/2))];
				}
				else
				{
					// fprintf(stdout, "ERROR: NormalizeBeads(): beadfiles %d-%d have 0 assignments of %d\n", i, i+3, m);
					assign_median[i/4][j][m] = 1;
				}
			} // end for j
		}
	}


	// calculate Euclidean distances (quality scores)
	for( i=0; i < curr_numobjs; i++ )
	{
		for( j=0; j < num_beadfiles; j+=4 )
		{ // for each cycle
			//assign_quals[cycle]
			//assignments[cycle]
			//assign_median[cycle][wxyz][ACGT]
			if( *(assignments[j/4]+i) < 4 )
			{ // A, C, G, or T
				*(assign_quals[j/4]+i) = sqrt((pow(*(normvals[j+0]+i) - assign_median[j/4][0][*(assignments[j/4]+i)], 2))+
						(pow(*(normvals[j+1]+i) - assign_median[j/4][1][*(assignments[j/4]+i)], 2))+
						(pow(*(normvals[j+2]+i) - assign_median[j/4][2][*(assignments[j/4]+i)], 2))+
						(pow(*(normvals[j+3]+i) - assign_median[j/4][3][*(assignments[j/4]+i)], 2)));
			}
			else
			{ // N
				*(assign_quals[(int)(floor(j/4))]+i) = 1;
			}	
		}
	}

#ifdef BCDEBUG0
	fprintf(stdout, "NormalizeBeads(): median coordinates:\n");
	fprintf(stdout, "Cycle\tw\tx\ty\tz\tdelta\n");
#endif
	for( i=0; i < num_beadfiles/4; i++ )
	{
		for( j=0; j < 4; j++ )
		{//A,C,G,T
#ifdef BCDEBUG0
			fprintf(stdout, "%02d  %c\t", i, base[j]);
			for( k=0; k < 4; k++ )
			{//w,x,y,z
				fprintf(stdout, "%0.05f\t", assign_median[i][k][j]);
			}
#endif
			// report median deltas; each basecall cluster (A, C, G, and T) has a 4-D
			// unit vector that defines its median; for a well-formed tetrahedron, three
			// of its components (the 3 which don't include the basecall, e.g. the C, G, 
			// and T components of the A median vector) will be small, and one component
			// (the one which is the basecall, e.g. the A component) should be large.  Thus,
			// the difference between the largest component and the second-largest component
			// serves as a metric of how 'uniform' the cluster is; we report one of these
			// for each basecall for each position, and then graphically display the mean
			// of the 4 (A, C, G, and T) for each position.  high values are good (lots of
			// separation, low values are bad (clusters close to multiple axes)
			temp_sort[0] = assign_median[i][0][j];
			temp_sort[1] = assign_median[i][1][j];
			temp_sort[2] = assign_median[i][2][j];
			temp_sort[3] = assign_median[i][3][j];
			quickSort(temp_sort, 4);
#ifdef BCDEBUG0
			fprintf(stdout, "%0.05f", temp_sort[3]-temp_sort[2]);
			fprintf(stdout, "\n");
#endif
			if(write_to_deltafile)
			{	
				fprintf(deltafile, "\t%0.05f", temp_sort[3]-temp_sort[2]);
			}
		}
	}
	if(write_to_deltafile)
	{
		fprintf(deltafile, "\n");
	}
	if(write_to_deltafile)
	{
		fflush(deltafile);
	}

#ifdef BCDEBUG0
	fprintf(stdout, "----------------------------------------------------------------\n");
#endif
	fprintf(stdout, "\n");


	for( i=0; i < num_beadfiles; i++ )
	{
		free(norm[i]);
		free(sortedbeadvals[i]);
	}
	for( i=0; i < 4; i++ )
	{
		free(temp_valarray[i]);
	}
}

void tetrahedron(double *data4d[MAX_BEADFILES], 
			char *assignments[MAX_BEADFILES],
			bool *normflags[MAX_BEADFILES],
			int *num_nonzero_beads,
			int num_beadfiles,
			double *data3d[MAX_BEADFILES],
			char *color3d[MAX_BEADFILES])
{

	int i;
	static double tetcoor[4][3] = 
	{
		{ 0.0000,  0.0000,  1.0000},
		{ 0.0000,  0.9428, -0.3333},
		{-0.8165, -0.4714, -0.3333},
		{ 0.8165, -0.4714, -0.3333}
	};

	for( i=0; i < num_beadfiles/4; i++ )
	{
		//FOR EACH CYCLE, CONVERT TO 3-D AND COLOR
	}

}



void quickSort(double numbers[], int array_size)
{
	qsort(numbers, array_size, sizeof(double), cmpNum);
}

double percentile_val(double a[], int n, int percentile)
{
	return kth_smallest(a, n, (int)(n/100) * percentile);
}

double kth_smallest(double a[], int n, int k)
{
	register int i,j,l,m;
	register double x;

	double *a_copy;
	double return_val;

	a_copy=(double*)malloc(n*sizeof(double));
	memcpy(a_copy, a, n*sizeof(double));

	l = 0; 
	m = n-1;

	while( l < m )
	{
		x = a_copy[k];
		i = l;
		j = m;
		do
		{
			while(a_copy[i] < x)
			{
				i++;
			}
			while(x < a_copy[j]) 
			{
				j--;
			}
			if(i <= j)
			{
				ELEM_SWAP(a_copy[i],a_copy[j]);
				i++;j--;
			}
		} while(i <= j);
		if( j < k) 
		{
			l = i;
		}
		if( k < i)
		{
			m = j;
		}
  }
  return_val = a_copy[k];
  free(a_copy);
  return return_val;
}

double quickSelect(double arr[], int n)
{
	int low, high;
	int median;
	int middle, ll, hh;
  
	double *a_copy;
	double return_val;
  
	a_copy = (double*)malloc(n*sizeof(double));
	memcpy(a_copy, arr, n*sizeof(double));
  
	low = 0; 
	high = n-1; 
	median=(low+high)/2;
	for(;;)
	{
		if(high <= low)
		{
			return_val = a_copy[median];
			free(a_copy);
			return return_val;
		}
		if( high == low+1 )
		{
			if( a_copy[low] > a_copy[high] ) 
			{
				ELEM_SWAP(a_copy[low],a_copy[high]);
			}
			return_val = a_copy[median];
			free(a_copy);
			return return_val;
		}
    
		middle=(low+high)/2;
		if( a_copy[middle] > a_copy[high]) 
		{
			ELEM_SWAP(a_copy[middle], a_copy[high]);
		}
		if(a_copy[low] > a_copy[high])
		{
			ELEM_SWAP(a_copy[low], a_copy[high]);
		}
		if( a_copy[middle] > a_copy[low])
		{
			ELEM_SWAP(a_copy[middle], a_copy[low]);
		}
    
		ELEM_SWAP(a_copy[middle], a_copy[low+1]);
    
		ll = low+1;
		hh = high;
		for(;;)
		{
			do ll++; 
			while (a_copy[low] > a_copy[ll]);
			do hh--; 
			while (a_copy[hh] > a_copy[low]);

			if( hh < ll) 
			{
				break;
			}
			ELEM_SWAP(a_copy[ll], a_copy[hh]);
		}
		ELEM_SWAP(a_copy[low], a_copy[hh]);
		if( hh <= median) 
		{
			low=ll;
		}
		if( hh >= median)
		{
			high=hh-1;
		}
	}
}


int cmpNum(const void *p1, const void *p2)
{
	if(*(double*)p1 < *(double*)p2)
	{
		return -1;
	}
	else if( *(double*)p1 > *(double*)p2 )
	{
		return 1;
	}
	return 0;
}

