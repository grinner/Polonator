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
#include "common.h"
#include "polonator_camerafunctions.h"
#include "Polonator_networkfunctions.h"
#include "polonator_maestro.h"
#include "Polonator_logger.h"
#include "Polonator_config.h"

#define WAIT usleep(1000)
#define DEBUG_POLONATORACQ /* for testing */

/* FUNCTION PROTOTYPES */
/**/
static void acquirer_callback(tHandle hCamera, ui32 dwInterruptMask, void *pvParams);
void average_images(short unsigned int* curr_img);
void exit_on_error(int, tHandle, int);
void send_initial_raw_images(int, int);
int send_FL_images(char *, int,int,int,int);
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

/* structure to hold info from callback */
typedef struct
{
  volatile int num_imgs; /* number of images received so far */
  volatile int image_ready; /* signals a new image has been received */
  volatile int readout_started;
} tPhxCallbackInfo;

/* Phoenix image buffer */
stImageBuff img_buffer;
ui32 PHX_buffersize; /* width of the framegrabber's circular image buffer */

/* camera handle */
tHandle hCamera = 0;

/* for error reporting */
char function_name[] = "Polonator-acquirer";
short unsigned int *autoexp_gain;

/* image averaging */
short unsigned int average_image[1000000]; /* holds the final averaged image */
int average_image_sums[1000000]; /* holds the accumulating sums for each pixel */
int curr_averageimgnum; /* counter of how many images have gone into the average thus far */
int num_to_average; /* number of images to average per position; command line arg */
int averaging_complete; /* signals that average_image is ready to be transmitted */




void exit_on_error(int m_sock, tHandle camhandle, int serv_sock){
  p_log_errorno("ERROR: we've had either a camera or network error; exiting prematurely and signalling PolonatorImager to retry the current scan");
  maestro_getstatus(m_sock);
  if(hCamera) PHX_CameraRelease(&camhandle);
  shutdown(serv_sock, 2);
  maestro_stop(m_sock);
  shutdown(m_sock, 2);
  p_log_errorno("ERROR: exiting");
  exit(10);
}


/* used to accumulate an average of multiple images; we call this during */
/* brightfield image acquisition to decrease noise so segmentation works better */ 
void average_images(short unsigned int* curr_img){
  int i;
  
  /* this is a global variable that tells us how many images */
  /* will have been averaged after the current call completes */
  /* calling function uses this to determine whether it's time */
  /* to transmit the image */
  curr_averageimgnum++; 
 
  if(curr_averageimgnum == num_to_average){ /* this image is the last one; add it, then compute avg */
    for(i=0; i<1000000; i++){
      *(average_image_sums + i) += *(curr_img + i);
      *(average_image + i) = (short unsigned int) (*(average_image_sums + i) / num_to_average);
    }
    curr_averageimgnum = 0; /* reset for next position */
    averaging_complete = 1; /* signal to transmit the data in average_image */
  }
  else if(curr_averageimgnum == 1){ /* first time for this position; overwrite previous data */
    averaging_complete = 0; /* reset so nothing is transmitted until we do num_to_avg images */
    for(i=0; i<1000000; i++){
      *(average_image_sums + i) = *(curr_img + i);
    }
  }
  else{
    for(i=0; i<1000000; i++){
      *(average_image_sums + i) += *(curr_img + i);
    }
  }
}


/* executed every time a BUFFER_READY event is registered by the Pheonix API */
/* it is very important to release the callback quickly so it can be re-called */
/* upon the next interrupt event; if it is not released quickly enough, */
/* BUFFER_READY events can be missed */
/* therefore we simply signal main() through sPCI->image_ready  that there */
/* is a new image, increment a counter, and DO NOTHING ELSE!!!*/
static void acquirer_callback(tHandle hCamera,
			      ui32 dwInterruptMask,
			      void *pvParams){

  tPhxCallbackInfo *sPCI = (tPhxCallbackInfo*) pvParams;
  (void) hCamera;

  /* event PHX_INTRPT_BUFFER_READY evaluates true when the framegrabber receives a */
  /*   full image from the camera; at this point, we can get a pointer to the image */
  /*   in the framegrabber's memory pool and copy it out for our own use */
  if(PHX_INTRPT_BUFFER_READY & dwInterruptMask){
    /* callback sets image_ready to true on execution, then main thread */

    /* resets it to false when it is finished with the image; therefore,*/
    /* it should always be false when the callback is executed */
    /* if it is not, it means code in the main thread (Polonator-acquirer.c) */
    /* for handling the previous image is still executing; warn the user because */
    /* ideally these two threads are not executing simultaneously */
    if(sPCI->image_ready){
      sprintf(log_string, "WARNING: Polonator-acquirer:callback: hit again before previous (%d) processed; images may be missed", sPCI->num_imgs);
      p_log_errorno(log_string);
      while(sPCI->image_ready){WAIT;}
    }
    
#ifdef DEBUG_POLONATORACQ
    p_log("PHX CALLBACK: IMAGE READY");
#endif

    /* holds total number of BUFFER_READY events since Polonator-acquirer started */
    sPCI->num_imgs++;

    /* signal main thread that a new image is ready for handling */
    sPCI->image_ready = 1;
  }
}

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

void send_initial_raw_images(int num_array, int clnt_sock)
{
  /* Network transfer declarations */
  int serv_sock; /* socket for the server (this machine) */
  int i = 0;
  int j = 0;
  int proc_portnum;
  char log_string[500];
  short unsigned int blank_image[1000000];

  /* Read file declarations */
  short unsigned int *baseimage;
  FILE *baseimgfp;
  char stagealign_rawimgfilename[500];
  /*prepare the network connection and send*/
  /* open the port, and wait for processor to connect */
  /*network_startserver(&serv_sock, &clnt_sock, proc_portnum);*/
  
 if((baseimage=(short unsigned int*)malloc(1000000 * sizeof(short unsigned int)))==NULL){
    fprintf(stderr, "ERROR:\tPolonator-acquirerTDI: unable to allocate memory for image buffer\n");
    exit(0);
  }

  while(i<num_array)
 {
  /* read and send the auto_exposure raw images */
  sprintf(stagealign_rawimgfilename, "/home/polonator/G.007/G.007_acquisition/stagealign/stagealign-image0_%d.raw", i);
  p_log_simple(stagealign_rawimgfilename);  
  
  if((baseimgfp = fopen(stagealign_rawimgfilename, "r"))==NULL){
    fprintf(stderr, "ERROR opening info file %s: ", stagealign_rawimgfilename);
    sprintf(log_string, "ERROR opening autoe file %s", stagealign_rawimgfilename);
    p_log(log_string);
    perror(NULL);
    exit(1);
  }
  sprintf(log_string, "STATUS:\t 1try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
  p_log(log_string);

  fread(baseimage, sizeof(short unsigned int), 1000000, baseimgfp);
  fclose(baseimgfp);
  sprintf(log_string, "STATUS:\t 2try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
  p_log(log_string);
  
  for(j=0; j<1000000; j++){
    blank_image[j]= *(baseimage + j);
  }
  j = 0;
  sprintf(log_string, "STATUS:\t 3try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
  p_log(log_string);

  while((network_waitforsend(clnt_sock)!=1))
  { 
		sprintf(log_string,"Waiting to send the autoexposure images");
		p_log(log_string);
  }
  sprintf(log_string,"Trying to send autoexposure images %d", i);
  p_log_simple(log_string);
  network_sendimage(clnt_sock, 0, 0, i, blank_image);
  sprintf(log_string,"Sent autoexposure images %d", i);
  p_log_simple(log_string);

i++;
  
}

}			      

int send_FL_images(char *mystring, int num_images, int num_array, int clnt_sock, int auto_exp_starting_gain)
{
  /* Network transfer declarations */
  int serv_sock; /* socket for the server (this machine) */
  int i = 0;
  int j = 0;
  int k = 0;
  int proc_portnum;
  char *string1;
  char log_string[500];
  short unsigned int blank_image[1000000];

  /* Read file declarations */
  short unsigned int *baseimage;
  FILE *baseimgfp;
  char stagealign_rawimgfilename[255];
  char stagealign_dir_name[255];

  int gain;
	
  /*prepare the network connection and send*/
  /* open the port, and wait for processor to connect 
  network_startserver(&serv_sock, &clnt_sock, proc_portnum);*/


  if((baseimage=(short unsigned int*)malloc(1000000 * sizeof(short unsigned int)))==NULL){
    fprintf(stderr, "ERROR:\tPolonator-acquirerTDI: unable to allocate memory for image buffer\n");
    exit(0);
  }

 string1 = mystring+strlen(mystring)-1;
 p_log(string1);

  sprintf(log_string, "string1 length %d", (unsigned int) strlen(string1));
  p_log(log_string);	

if(strcmp(string1, "G") == 0)
{
  sprintf(log_string, "success_%d.raw", 0);
  p_log(log_string);

  sprintf(stagealign_dir_name, "/home/polonator/G.007/G.007_acquisition/autoexp_FL_images/fam/");
  p_log(stagealign_dir_name);  
  
}

else if(strcmp(string1, "A") == 0)
{
  sprintf(log_string, "success_%d.raw", (int) 0);
  p_log(log_string);

  sprintf(stagealign_dir_name, "/home/polonator/G.007/G.007_acquisition/autoexp_FL_images/cy3/");
  p_log(stagealign_dir_name);  
  
}

else if(strcmp(string1, "C") == 0)
{
  sprintf(log_string, "success_%d.raw", 0);
  p_log(log_string);

  sprintf(stagealign_dir_name, "/home/polonator/G.007/G.007_acquisition/autoexp_FL_images/txred/");
  p_log(stagealign_dir_name);  
  
}

else if(strcmp(string1, "T") == 0)
{
  sprintf(log_string, "success_%d.raw", 0);
  p_log(log_string);

  sprintf(stagealign_dir_name, "/home/polonator/G.007/G.007_acquisition/autoexp_FL_images/cy5/");
  p_log(stagealign_dir_name);  
  
}

  while(i<num_array)
 {
  /* read and send the FL autoe images */
  for(k=0;k<num_images;k++){
	  sprintf(stagealign_rawimgfilename,"%s%d_image_%d.raw",stagealign_dir_name,i,(auto_exp_starting_gain+k*10));

	  if((baseimgfp = fopen(stagealign_rawimgfilename, "r"))==NULL){
	    fprintf(stderr, "ERROR opening info file %s: ", stagealign_rawimgfilename);
	    sprintf(log_string, "ERROR opening autoe file %s", stagealign_rawimgfilename);
	    p_log(log_string);
	    perror(NULL);
	    exit(1);
	  }
	  sprintf(log_string, "STATUS:\t 1try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
	  p_log(log_string);

	  fread(baseimage, sizeof(short unsigned int), 1000000, baseimgfp);
	  fclose(baseimgfp);
	  sprintf(log_string, "STATUS:\t 2try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
	  p_log(log_string);
	  
	  for(j=0; j<1000000; j++){
	    blank_image[j]= *(baseimage + j);
	  }
	  j = 0;
	  sprintf(log_string, "STATUS:\t 3try to send the autoexposure images %s to processor, port %d...", stagealign_rawimgfilename, proc_portnum);
	  p_log(log_string);

	  while((network_waitforsend(clnt_sock)!=1))
	  { 
			sprintf(log_string,"Waiting to send the autoexposure images");
			p_log(log_string);
	  }
	  sprintf(log_string,"Trying to send autoexposure FL images %d for lane %d",k, i);
	  p_log_simple(log_string);
	  network_sendimage(clnt_sock, 0, i, k, blank_image);
	  sprintf(log_string,"Sent autoexposure FL images %d for lane %d", k, i);
	  p_log_simple(log_string);
 }
i++; 
}

for(k=0;k<num_array;k++)
{
	i =0;
	  while(i<2)
	  {
			i = network_waitforsend(clnt_sock);
	  }
	  autoexp_gain[k] = (i-97)*10+auto_exp_starting_gain;
}
for(k=0;k<num_array;k++)
{
	  sprintf(log_string,"the autoexposure gain for lane %d to use will be %d", k, autoexp_gain[k]);
	  p_log_simple(log_string);
}
}	


