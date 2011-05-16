/* =============================================================================
//
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Polonator_logger.c: functionality for logging events
//
// Release 1.0 -- 04-15-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include "logger.h"

FILE *fp = NULL;
int disp_flag=1;


/* MUST be called before any calls to p_log or p_log_errorno
   if logging to file is desired.  If not called, messages will
   be printed to the stdout only

   To suppress output on stdout, set df == 0
*/
void start_logger(char *logfilename, int df)
{
    if (fp = fopen(logfilename, "a") != NULL) {
        fprintf(stdout, "Polonator_logger starting with logfile <%s>...\n", logfilename);
        fprintf(fp, "-------------------------------------------------------------------------------------\n");
        if(df == 0){ /* default is 1 */
            disp_flag = df;
        }
        fflush(stdout);
        fflush(fp);
    }
}

void close_logger(void)
{
    fclose(fp);
}


void set_disp(int d)
{
    if(d != 0 ){
        disp_flag = 1;
    }
    else{
        disp_flag = 0;
    }
}


/* Outputs message to log file and/or stdout */
void p_log(char *message){
/* time_t curr_time;
  struct timeval tv;
  char timestring[50];

  gettimeofday(&tv, NULL);
  curr_time = tv.tv_sec;

  strftime(timestring, 50, "%m-%d-%Y %T.", localtime(&curr_time));
 if(fp != NULL){
    fprintf(fp, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
  }

  if(disp_flag){
    fprintf(stdout, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
  }
  fflush(fp);
*/
}

/* Outputs message to log file and (optionally) stdout */
void p_log_simple(char *message){
    time_t curr_time;
    struct timeval tv;
    char timestring[50];

    gettimeofday(&tv, NULL);
    curr_time = tv.tv_sec;

    strftime(timestring, 50, "%m-%d-%Y %T.", localtime(&curr_time));
    /*  if(fp != NULL){
    fprintf(fp, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
    }
    */
    if(disp_flag){
        fprintf(stdout, "%s%06ld\t%s\n", timestring, tv.tv_usec, message);
    }
    fflush(fp);

}

/* Outputs message to log file and/or stdout
   Use this function when an error code is present in
   errno -- p_log_errorno() will print out the test message
   associated with the errno
*/
void p_log_errorno(char *message){
    time_t curr_time;
    struct timeval tv;
    char timestring[50];

    gettimeofday(&tv, NULL);
    curr_time = tv.tv_sec;

    strftime(timestring, 50, "%m-%d-%Y %T.", localtime(&curr_time));
    if(fp != NULL){
        fprintf(fp, "%s%06ld\t%s: %s\n", timestring, tv.tv_usec, message, strerror(errno));
    }
    if(disp_flag){
        fprintf(stdout, "%s%06ld\t%s: %s\n", timestring, tv.tv_usec, message, strerror(errno));
    }
    fflush(fp);
}

