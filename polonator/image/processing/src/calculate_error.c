#include <stdlib.h>
#include <stdio.h>

#define NUM_QBINS 1000
#define MAX_READLENGTH 255
#define CFD_NUMBINS 100
//#define PRINT_BYPOSITION 1
#define PRINT_FULL_QTABLE 1

int main(int argc, char *argv[])
{
	int num_bases;
	int num_cols;

	int *correct[NUM_QBINS];
	int *incorrect[NUM_QBINS];
	int *correctpos;
	int *incorrectpos;
	int cfd_correct[CFD_NUMBINS];
	int cfd_incorrect[CFD_NUMBINS];

	int *qscores;
	int i,j;

	char current_read[MAX_READLENGTH];
	char current_qual[5];

	int best_base[4];
	char base_char[] = "ACGT";

	int curr_val;
	char curr_char;
	int max_count;
	int max_base;
	int call_iscorrect;
	int bin_correct;
	int bin_incorrect;
	int cfd_currbin;
	int cfd_binsize;
	int cfd_currbinsize;
	int cfd_curr_correct, cfd_curr_incorrect;

	long int total_basecalls;

	num_cols = 0;
	while((curr_char=fgetc(stdin))!='\n')
	{
		if(curr_char == '\t')
		{
			num_cols++;
		}
	}
	num_cols++;
	num_bases = num_cols - 5;
	current_qual[4] = '\0'; // this array is treated as a string

	qscores = (int*)malloc(num_bases*sizeof(int));
	correctpos = (int*)malloc(num_bases*sizeof(int));
	incorrectpos = (int*)malloc(num_bases*sizeof(int));
	for(i=0; i < num_bases; i++)
	{
		*(correctpos+i)=0;
		*(incorrectpos+i)=0;
	}
	for(i=0; i < CFD_NUMBINS; i++)
	{
		cfd_correct[i]=0;
		cfd_incorrect[i]=0;
	}
	for(i=0; i < NUM_QBINS; i++)
	{
		correct[i] = (int*)malloc(num_bases*sizeof(int));
		incorrect[i] = (int*)malloc(num_bases*sizeof(int));
		for(j=0; j < num_bases; j++)
		{
			*(correct[i]+j)=0;
			*(incorrect[i]+j)=0;
		}
	}

	total_basecalls=0;
	while(!feof(stdin))
	{
		// load current read into memory
		fscanf(stdin, "%d\t%d\t%d\t%d\t",
				&curr_val,
				&curr_val,
				&curr_val,
				&curr_val);
		for(i=0; i < num_bases; i++)
		{
			current_read[i] = fgetc(stdin);
		}
		for(i=0; i < num_bases; i++)
		{
			fgetc(stdin); //discard tab delimiter
			for(j=0; j < 4; j++)
			{
				current_qual[j]=fgetc(stdin);
			}
				*(qscores+i) = atoi(current_qual);
		}


		// determine whether the current read should be A, C, G, or T
		// by simple majority
		for(i=0; i < 4; i++)
		{
			best_base[i] = 0;
		}
		for(i=0; i < num_bases; i++)
		{
			if(current_read[i] == '.'){;}
			else
			{
				if(current_read[i] == 'A') best_base[0]++;
				else if(current_read[i] == 'C') best_base[1]++;
				else if(current_read[i] == 'G') best_base[2]++;
				else best_base[3]++;
			}
		}
		max_count = 0;
		max_base = 0;
		for(i=0; i < 4; i++)
		{
			if(best_base[i] > max_count)
			{
				max_count = best_base[i];
				max_base = i;
			}
		}

		// now, tally and report correct and incorrect basecalls for the current read
		for(i=0; i<num_bases; i++)
		{
			if(current_read[i]!='.')
			{

				if(current_read[i] == base_char[max_base])
				{
					call_iscorrect=1;
					(*(correctpos+i))++;
					(*(correct[*(qscores+i)]+i))++;
				}
				else
				{
					call_iscorrect = 0;
					(*(incorrectpos+i))++;
					(*(incorrect[*(qscores+i)]+i))++;
				}
				total_basecalls++;

				//	fprintf(stdout, "%d\tcall:%c\texpected:%c\tquality:%04d\n",
				//call_iscorrect,
				//current_read[i],
				//base_char[max_base],
				//*(qscores+i));
			}
		}
	}

	cfd_binsize=(int)(total_basecalls/CFD_NUMBINS);
	cfd_currbin=0;
	cfd_currbinsize=0;
	for(i=0; i<NUM_QBINS; i++)
	{
#ifdef PRINT_FULL_QTABLE
		fprintf(stdout, "%04d", i);
#endif

		bin_correct=0;
		bin_incorrect=0;
		for(j=0; j < num_bases; j++)
		{
			bin_correct+=*(correct[i]+j);
			bin_incorrect+=*(incorrect[i]+j);
		}

#ifdef PRINT_FULL_QTABLE
		fprintf(stdout, "\t%d\t%d", bin_incorrect, bin_correct+bin_incorrect);
		if( (bin_correct+bin_incorrect) == 0 )
		{
			fprintf(stdout, "\t-1\n");
		}
		else
		{
			fprintf(stdout, "\t%f\n", 
				(double)bin_incorrect / (double)(bin_correct+bin_incorrect));
		}
#endif

		if( (cfd_currbinsize + bin_correct+bin_incorrect) > cfd_binsize )
		{
			if(cfd_currbin==CFD_NUMBINS-1)
			{
				cfd_currbin=CFD_NUMBINS-1;
			}
			else
			{
				cfd_currbin++;
			}
			cfd_currbinsize = 0;
		}
		cfd_correct[cfd_currbin]+=bin_correct;
		cfd_incorrect[cfd_currbin]+=bin_incorrect;
		cfd_currbinsize+=(bin_correct+bin_incorrect);
	}

	for(i=0; i < CFD_NUMBINS; i++)
	{
		cfd_curr_correct=0;
		cfd_curr_incorrect=0;
		for(j=0; j < (i+1); j++)
		{
			cfd_curr_correct+=cfd_correct[j];
			cfd_curr_incorrect+=cfd_incorrect[j];
		}
		if( (cfd_curr_correct+cfd_curr_incorrect) == 0 )
		{
			fprintf(stdout, "%d\t%d\t%d\t-1\n",
					i,
					cfd_curr_incorrect,
					cfd_curr_correct+cfd_curr_incorrect);
		}
		else
		{
			fprintf(stdout, "%d\t%d\t%d\t%f\n",
					i,
					cfd_curr_incorrect,
					cfd_curr_correct+cfd_curr_incorrect,
					(double)cfd_curr_incorrect/(double)(cfd_curr_correct+cfd_curr_incorrect));
		}
	}

	/*      
	for(i=0; i<CFD_NUMBINS; i++)
	{
		fprintf(stdout, "%d\t%d\t%d\t",
				i,
				cfd_incorrect[i],
				cfd_correct[i]+cfd_incorrect[i]);

		if((cfd_correct[i]+cfd_incorrect[i])==0)
		{
			fprintf(stdout, "-1\n");
		}
		else
		{
			fprintf(stdout, "%f\n",
			(double)cfd_incorrect[i]/(double)(cfd_correct[i]+cfd_incorrect[i]));
		}
	}
    */
#ifdef PRINT_BYPOSITION
	for(i=0; i < num_bases; i++)
	{
		fprintf(stdout, "%d\t%d\t%d",
			i,
			*(incorrectpos+i),
			*(incorrectpos+i)+*(correctpos+i));
		if((*(correctpos+i) + *(incorrectpos+i)) == 0)
		{
			fprintf(stdout, "\t-1\n");
		}
		else
		{
			fprintf(stdout, "\t%f\n",
			(double)*(incorrectpos+i) / (double)(*(incorrectpos+i)+*(correctpos+i)));
		}
	}
#endif
    
	free(qscores);
	for(i=0; i < NUM_QBINS; i++)
	{
		free(correct[i]);
		free(incorrect[i]);
	}


}  
