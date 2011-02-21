/* =============================================================================
//
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Polonator-acquirer.c: used to collect images from the camera as the scan
// routine is running on the Maestro controller; sends images over TCP
// to the processor
//
// Release 1.0 -- 05-06-2008
// Release 2.0 -- 01-07-2009 Modified for controller software V1 [GP]
// Release 3.0 -- DMD acquirer [Chao] 01-25-2010
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include <sys/time.h>
#include <time.h>
//#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include "network_functions.h"
#include "logger.h"
//#include "Polonator_config.h"

#define WAIT usleep(1000)
#define DEBUG_POLONATORACQ /* for testing */

/* FUNCTION PROTOTYPES */
/**/
void send_DMD_register_image(int, int, int);

/* DMD functions and variables */
void py_send_DMD_register_image(int,int);
int py_get_X_offset(void);
int py_get_Y_offset(void);
void py_start_network_server(void);
void py_stop_network_server(void);

void py_force_network_shutdown(void); /* Added by NC 2/3/2010*/

int DMD_x_offset = 0;
int DMD_y_offset = 0;
int DMD_serv_sock; /* socket for the server (this machine) */
int DMD_clnt_sock; /* socket for the client (the processing machine) */
/* DMD functions and variables finished*/

/* GLOBAL DECLARATIONS */
/**/
char log_string[500];


/* for error reporting */
char function_name[] = "polonator_proc_DMD";


/* start network server to PROC machine */
void py_start_network_server(void){
     network_startserver(&DMD_serv_sock, &DMD_clnt_sock, 4000);
     fprintf(stdout, "The network socket is: %d\n" , DMD_serv_sock);
}
/* close network server to PROC machine */
void py_stop_network_server(void){
     network_stopserver(DMD_serv_sock);
}

void py_force_network_shutdown(void){
	//int stop_socket = 7;
	//network_stopserver(stop_socket);
	network_shutdown();
}

/* send DMD image (/DMD/image.raw) to PROC machine */
void py_send_DMD_register_image(int lane_num, int image_num){
     send_DMD_register_image(lane_num, image_num, DMD_clnt_sock);
}
/* return DMD image x_offset */
int py_get_X_offset(void){
     return DMD_x_offset;
}
/* return DMD image y_offset */
int py_get_Y_offset(void){
     return DMD_y_offset;     
}

void send_DMD_register_image(int array_num, int image_num, int clnt_sock)
{
  int serv_sock; /* socket for the server (this machine) */
  int i = 0;
  int k,j = 0;
  int proc_portnum;
  int x_offset,y_offset;
  char log_string[500];
  short unsigned int blank_image[1000000];

  /* Read file declarations */
  short unsigned int *baseimage;
  FILE *baseimgfp;
  char stagealign_rawimgfilename[500];
  
 if((baseimage=(short unsigned int*)malloc(1000000 * sizeof(short unsigned int)))==NULL){
    fprintf(stderr, "ERROR:\tPolonator-acquirerTDI: unable to allocate memory for image buffer\n");
    exit(0);
  }

  /* read and send the DMD register image */
  sprintf(stagealign_rawimgfilename, "/home/polonator/G.007/G.007_acquisition/DMD/image.raw");
  p_log_simple(stagealign_rawimgfilename);  
  
  if((baseimgfp = fopen(stagealign_rawimgfilename, "r"))==NULL){
    fprintf(stderr, "ERROR opening DMD image file %s: \n", stagealign_rawimgfilename);
    sprintf(log_string, "ERROR opening DMD image file %s", stagealign_rawimgfilename);
    p_log(log_string);
    perror(NULL);
    exit(1);
  }

  fread(baseimage, sizeof(short unsigned int), 1000000, baseimgfp);
  fclose(baseimgfp);
  
  for(j=0; j<1000000; j++){
    blank_image[j]= *(baseimage + j);
  }

  while((network_waitforsend(clnt_sock)!=1))
  { 
  }
  network_sendimage(clnt_sock, 0, array_num, image_num, blank_image);
  sprintf(log_string,"Sent DMD register image for lane %d, image %d", array_num, image_num);
  p_log(log_string);
  i =-1;
  k=0;
	  while(k<2)
	  {
			i = network_waitforDMD(clnt_sock);
			if(i>-1){
				if(k==0) DMD_x_offset = i-60;								
				else if(k==1) DMD_y_offset = i-60;
				k++;
			}

	  }
       sprintf(log_string,"DMD image-registration for lane %d, image %d, x_offset is %d, y_offset is %d", array_num, image_num, DMD_x_offset, DMD_y_offset);
	  p_log_simple(log_string);
	  free(baseimage);
}			      


