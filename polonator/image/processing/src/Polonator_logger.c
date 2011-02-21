#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include "Polonator_logger.h"

FILE *fp = NULL;
int disp_flag=1;


/* MUST be called before any calls to p_log or p_log_errorno
   if logging to file is desired.  If not called, messages will
   be printed to the stdout only

   To suppress output on stdout, set df == 0
*/
void start_logger(char *logfilename, int df)
{
	fp = fopen(logfilename, "a");
	fprintf(stdout, "Polonator_logger starting with logfile <%s>...\n", logfilename);
	fprintf(fp, "-------------------------------------------------------------------------------------\n");
	if(df == 0)
	{ /* default is 1 */
		disp_flag = df;
	}
}

void set_disp(int d)
{
	if(d != 0)
	{
		disp_flag = 1;
	}
	else
	{
		disp_flag = 0;
	}
}

void change_logfile(char *logfilename){
	char log_string[255];

	sprintf(log_string, "**    CHANGING LOGFILE TO %s", logfilename);
	p_log(log_string);

	if(fp != NULL)
	{
		fclose(fp);
		fp = fopen(logfilename, "a");
	}
}


/* Outputs message to log file and (optionally) stdout */
void p_log(char *message)
{ 
	time_t curr_time;
	struct timeval tv;
	char timestring[50];

	gettimeofday(&tv, NULL);
	curr_time = tv.tv_sec;

	strftime(timestring, 50, "%m-%d-%Y %T.", localtime(&curr_time));
	if(fp != NULL)
	{
		fprintf(fp, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
	}
	if(disp_flag)
	{
		fprintf(stdout, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
	}
	fflush(fp);
}


/* Outputs message to log file and (optionally) stdout
   Use this function when an error code is present in
   errno -- p_log_errorno() will print out the test message
   associated with the errno
*/
void p_log_errorno(char *message)
{ 
	time_t curr_time;
	struct timeval tv;
	char timestring[50];

	gettimeofday(&tv, NULL);
	curr_time = tv.tv_sec;

	strftime(timestring, 50, "%m-%d-%Y %T.", localtime(&curr_time));
	if(fp != NULL)
	{
		fprintf(fp, "%s%06ld\t%s: %s\n", timestring, tv.tv_usec, message, strerror(errno));
	}
	if(disp_flag)
	{
		fprintf(stdout, "%s%06ld\t%s: %s\n", timestring, tv.tv_usec, message, strerror(errno));
	}
	fflush(fp);
}

