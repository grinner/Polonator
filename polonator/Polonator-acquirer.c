/* =============================================================================
S// 
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
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include <sys/time.h>
#include <time.h>
#include "common.h"
#include "polonator_camerafunctions.h"  //NC 09-20-2010
#include "Polonator_networkfunctions.h"
#include "polonator_maestro.h" //NC 09-20-2010
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
int send_FL_images(char *, int, int);

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

/* image averaging */
short unsigned int average_image[1000000]; /* holds the final averaged image */
int average_image_sums[1000000]; /* holds the accumulating sums for each pixel */
int curr_averageimgnum; /* counter of how many images have gone into the average thus far */
int num_to_average; /* number of images to average per position; command line arg */
int averaging_complete; /* signals that average_image is ready to be transmitted */


int main(int argc, char *argv[]){

  short unsigned int test[1000000];
  char *array_string;

  /* Camera declarations */
  tPhxCallbackInfo sPCI; /* structure to hold image_ready flag and current image counter */
  etStat eStat; /* holds 'status' information from Phoenix API calls */
  etParamValue dw; 
  ui32 dwNumberOfImages;
  etParamValue eParamValue;
  char PHX_configfilename[255];

  /* Network transfer declarations */
  int serv_sock; /* socket for the server (this machine) */
  int clnt_sock; /* socket for the client (the processing machine) */
  short unsigned int blank_image[1000000]; /* blank image used to 'prime' network connection */
  int i;
  int proc_portnum;
  int autoe_gain;

  /* Maestro declarations */
  int m_sock; /* socket for the Maestro controller */

  /* Image declarations */
  short unsigned int curr_fc;
  short unsigned int curr_array;
  short unsigned int curr_img;
  short unsigned int TOTAL_FCS=1; /*DO NOT CHANGE THIS*/
  short unsigned int TOTAL_ARRAYS;
  short unsigned int TOTAL_IMGS;
  int first_average_image=1;

  /* Timing declarations */
  struct timeval tv1; /* structure to hold seconds and microseconds for time of last image */
  struct timeval tv2; /* structure to hold seconds and microseconds for current time */
  int wait_time; /* computed to determine how long it's been since the last image arrived */
  int image_timeout; /* after image_timeout seconds, assume the image isn't coming, and acquire it manually */

  FILE *outfile;
  int fcnum;
  char log_string[255];
  char config_value[255];
  char logfilename[255];

  int caught_readout;
  int auto_exposure = 0;


  /* Open config file */
  config_open("/home/polonator/G.007/G.007_acquisition/src/polonator-acq.cfg");

  /* Initialize variables */
  dw = (etParamValue) (0 | 
		       /*		       PHX_INTRPT_GLOBAL_ENABLE |*/ 
		       PHX_INTRPT_BUFFER_READY |
		       PHX_INTRPT_FRAME_START); /* only trap buffer_ready events */

  if(!config_getvalue("logfilename", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key logfilename) returned no value\n");
    exit(0);
  }
  strcpy(logfilename, config_value);
  strcat(logfilename, ".");
  strcat(logfilename, argv[1]);
  strcat(logfilename, ".log");
  start_logger(logfilename, 1);

  if(!config_getvalue("imgs_per_array", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key imgs_per_array) returned no value\n");
    exit(0);
  }
  TOTAL_IMGS = atoi(config_value);

  if(!config_getvalue("image_timeout", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key image_timeout) returned no value\n");
    exit(0);
  }
  image_timeout = atoi(config_value);

  if(!config_getvalue("PHX_buffersize", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key PHX_buffersize) returned no value\n");
    exit(0);
  }
  PHX_buffersize = atoi(config_value);

  if(!config_getvalue("proc_portnum", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key proc_portnum) returned no value\n");
    exit(0);
  }
  proc_portnum = atoi(config_value);

  if(!config_getvalue("PHX_configfilename", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key PHX_configfilename) returned no value\n");
    exit(0);
  }
  strcpy(PHX_configfilename, config_value);

  if(!config_getvalue("auto_exposure", config_value)){
    fprintf(stderr, "ERROR:\tPolonator-acquirer: config_getval(key proc_portnum) returned no value\n");
    exit(0);
  }
  auto_exposure = atoi(config_value);

  if(argc == 7){ /* number of images to average was specified */
    sprintf(log_string, "%s called with %d args: <%s> <%d> <%d> <%d> <%d> <%d>", argv[0], argc-1, argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    p_log(log_string);
    num_to_average = atoi(argv[6]); /* num_to_avg consecutive images should be averaged together, then transmitted */
    if(num_to_average == 1){
      num_to_average = 0;
    }
  }
  else if(argc == 6){ /* number of images to average was not specified; assume no averaging */
    sprintf(log_string, "%s called with %d args: <%s> <%d> <%d> <%d> <%d>", argv[0], argc-1, argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    p_log(log_string);
    num_to_average = 0; /* don't do image averaging */
  }
  else{
    fprintf(stdout, "ERROR: %s called with %d args, must be called as\n", argv[0], argc-1);
    fprintf(stdout, "       %s <cycle_name> <integration time in seconds> <EM gain> <flowcell number> <number of arrays> <num_imgs_to_average>  OR\n", argv[0]);
    fprintf(stdout, "       %s <cycle_name> <integration time in seconds> <EM gain> <flowcell number> <number of arrays> IF no image averaging desired.\n", argv[0]);
    exit(1);
  }
  fcnum = atoi(argv[4]);
  TOTAL_ARRAYS = atoi(argv[5]);

 


  /*-------------------------------------------------------------------------------------------------------------
  /
  / CAMERA SETUP
  /*/
  /* configure framegrabber */
  p_log_simple("STATUS:\tPolonator_acquirer: PHX_CameraConfigLoad");
  eStat = PHX_CameraConfigLoad(&hCamera, PHX_configfilename, (etCamConfigLoad)(PHX_BOARD_AUTO|PHX_DIGITAL|PHX_NO_RECONFIGURE), PHX_ErrHandlerDefault);
  check_for_error(eStat, function_name, "PHX_CameraConfigLoad()");

  /* specify which events to recognize */
  p_log_simple("STATUS:\tPolonator_acquirer: PHX_ParameterSet(PHX_INTRPT_SET)");
  eStat = PHX_ParameterSet(hCamera, PHX_INTRPT_SET, &dw);
  check_for_error(eStat, function_name, "PHX_ParameterSet(PHX_INTRPT_SET)");

  /* setup camera for external trigger mode */
  p_log_simple("STATUS:\tPolonator-acquirer: setup camera for external triggering...");
  init_camera_external_trigger(hCamera);

  /* set exposure and gain on camera */
  sprintf(log_string, "STATUS:\tPolonator-acquirer: set camera integration time to %f seconds", atof(argv[2])/1000);
  p_log(log_string);
  set_exposure(hCamera, atof(argv[2])/1000);

  sprintf(log_string, "STATUS:\tPolonator-acquirer: set camera EM gain to %d (out of 255)", atoi(argv[3]));
  p_log(log_string);
  set_gain(hCamera, atoi(argv[3])); 

  /* setup event context */
  p_log("STATUS:\tPolonator-acquirer: setup event context...");
  memset(&sPCI, 0, sizeof(tPhxCallbackInfo));
  sPCI.num_imgs = 0;
  sPCI.image_ready = 0;
  sPCI.readout_started = 0;
  eStat = PHX_ParameterSet( hCamera, PHX_EVENT_CONTEXT, (void *) &sPCI );
  check_for_error(eStat, function_name, "PHX_ParameterSet(PHX_EVENT_CONTEXT)");

  /* setup capture */
  p_log_simple("STATUS:\tPolonator-acquirer: setup capture...");
  eStat = PHX_ParameterSet( hCamera, PHX_ACQ_NUM_IMAGES, (void *) &PHX_buffersize );
  check_for_error(eStat, function_name, "PHX_ParameterSet(PHX_ACQ_NUM_IMGS)");
  /*
  /-------------------------------------------------------------------------------------------------------------
  */

 
  /*-------------------------------------------------------------------------------------------------------------
  //
  // NETWORK TRANSFER SETUP
  /*/
  /* open the port, and wait for processor to connect */



  if(auto_exposure==1){
	  network_startserver(&serv_sock, &clnt_sock, proc_portnum);

	  array_string = argv[1];

	  p_log(array_string);

	  if((strncmp(argv[1],"0",1)==0)||(strncmp(argv[1],"1",1)==0)){
		  p_log(array_string);
		  send_initial_raw_images(1,clnt_sock);
	  }
	  else {
		   sprintf(log_string, "STATUS:\tPolonator-acquirer: running autoexposure, server sending filename %s to processor, port %d...", argv[1], proc_portnum);
		   p_log_simple(log_string);
 

		   /* now send the filename for the current image set */
		  sprintf(log_string, "STATUS:\tPolonator-acquirer: server sending filename %s to processor, port %d...", argv[1], proc_portnum);
		  p_log(log_string);
		  network_sendfilename(clnt_sock, argv[1]);

		  /* close connection, then re-open for flowcell number */
		  network_stopserver(serv_sock);
		  network_startserver(&serv_sock, &clnt_sock, proc_portnum);

		   autoe_gain = send_FL_images(array_string,15,clnt_sock);
	  	  set_gain(hCamera, autoe_gain);

		  sprintf(log_string, "STATUS:\tPolonator-acquirer: using the auto-exposure gain %d...", autoe_gain);
		  p_log_simple(log_string);
	  }

  	network_stopserver(serv_sock); 
 }



  p_log_simple("STATUS:\tPolonator-acquirer: server waiting for processor to connect...");
  network_startserver(&serv_sock, &clnt_sock, proc_portnum);

  /* now send the filename for the current image set */
  sprintf(log_string, "STATUS:\tPolonator-acquirer: server sending filename %s to processor, port %d...", argv[1], proc_portnum);
  p_log(log_string);
  network_sendfilename(clnt_sock, argv[1]);

  /* close connection, then re-open for flowcell number */
  network_stopserver(serv_sock);
  network_startserver(&serv_sock, &clnt_sock, proc_portnum);
  sprintf(log_string, "STATUS:\tPolonator-acquirer: server sending flowcell %d to processor, port %d...", fcnum, proc_portnum);
  p_log(log_string);
  network_sendfcnum(clnt_sock, fcnum);
  


  /* close connection, then re-open for image xfer */
  network_stopserver(serv_sock);




  sprintf(log_string, "STATUS:\tPolonator-acquirer: server establishing image transfer connection to processor, port %d...", proc_portnum); 
  p_log_simple(log_string);
  network_startserver(&serv_sock, &clnt_sock, proc_portnum);

  sprintf(log_string, "STATUS:\tPolonator-acquirer: server establishing image transfer connection to processor, port %d...", proc_portnum); 
  p_log(log_string);

  for(i=0; i<1000000; i++){
    blank_image[i]=0;
  }
  p_log_simple("STATUS:\tPolonator-acquirer: waiting for processor to signal ready to begin");
  if(network_waitforsend(clnt_sock)!=1){
    p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
    exit_on_error(m_sock, hCamera, serv_sock);
  }
  p_log_simple("STATUS:\tPolonator-acquirer: processor ready to receive first image");
  network_sendimage(clnt_sock, 0, 0, 0, blank_image);
  /*
  //-------------------------------------------------------------------------------------------------------------
  */

  /*-------------------------------------------------------------------------------------------------------------
  //
  // MAESTRO SETUP
  /*/
  p_log("STATUS:\tPolonator-acquirer: opening connection to Maestro...");
  maestro_open(&m_sock);
  /*
  //-------------------------------------------------------------------------------------------------------------
  */


  

  /*-------------------------------------------------------------------------------------------------------------
  //
  // IMAGE CAPTURE AND TRANSMIT
  /*/
  /* start capture on framegrabber */
  p_log("STATUS:\tPolonator-acquirer: starting capture...");
  eStat = PHX_Acquire( hCamera, PHX_START, (void*) acquirer_callback);
  check_for_error(eStat, function_name, "PHX_Acquire(PHX_START)");
  
  /* initialize counters for image information transmitted to processor */
  curr_fc = 0;
  curr_array = 0;
  curr_img = 0;

  /* initialize image averaging vars */
  curr_averageimgnum = 0;
  averaging_complete = 0;


  sleep(5);

  caught_readout=0;

/*added
maestro_setcolor(m_sock, "cy5");
while(!py_snapReceived()){;}
maestro_setcolor(m_sock, "cy3");
  maestro_snap(m_sock,60 * 1000.0, 1);
while(!py_snapReceived()){;}
maestro_setcolor(m_sock, "txred");
  maestro_snap(m_sock,60 * 1000.0, 1);
while(!py_snapReceived()){;}
*/	

  
 /* send_initial_raw_images(8,clnt_sock); */

  /* capture all images for the current imaging cycle */
  /* we're at the end once curr_fc has been incremented */
  gettimeofday(&tv2, NULL);
  sPCI.readout_started = 1; 

  while(curr_fc < 1){

    gettimeofday(&tv1, NULL);
    wait_time = tv1.tv_sec - tv2.tv_sec;

    /* if we're imaging two flowcells, during brightfield imaging fcnum will be 2 or 3 */
    /* for 1 or 2 flowcells, respectively; now that we've told the processing computer */
    /* it should expect two flowcells, change the value so it reflects flowcell number */
    /* 0 or 1 when inserted into each image header */
    if(fcnum > 1){
      fcnum = fcnum - 2;
    }

    /* this should never evaluate true, but if it does, for debugging, determine whether it
       was caused by the controller or the framegrabber
    */
/*time out removed on 100609    if(wait_time > image_timeout){
      p_log_errorno("ERROR: have waited a long time; any socket errors?");
      p_log("ERROR: we've been waiting a very long time for an image to arrive; Maestro status:");
      exit_on_error(m_sock, hCamera, serv_sock);
    }
*/    


    /* the acquisition cycle is as follows:
        -wait for image readout from previous acquisition to begin
	-wait for processor to be ready to handle image being read out
	-set iprdy flag on controller so imaging continues at next position
	-wait for readout to framegrabber to complete (image_ready interrupt)
	-send image to processor
	-loop
    */

     /*callback hit; readout has begun on the current image*/	 
    if(sPCI.readout_started){
      caught_readout=1;
      sprintf(log_string, "STATUS:\tPolonator-acquirer: main loop evaluated readout_started==true for image %d", sPCI.num_imgs);
      p_log(log_string);
      sPCI.readout_started=0;
      if( (sPCI.num_imgs) && (num_to_average == 0)){
	p_log("STATUS:\tPolonator-acquirer: wait for processor ready to receive image");
	if(network_waitforsend(clnt_sock)!=1){
	  p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
	  exit_on_error(m_sock, hCamera, serv_sock);
	}
      }
      p_log("STATUS:\tPolonator-acquirer: processor ready to receive image");
      /* set controller flag so acquisition continues*/
      p_log("STATUS:\tPolonator-acquirer: signalling controller ready to start integration of next image");
      maestro_setflag(m_sock);
      p_log("STATUS:\tPolonator-acquirer: finished signalling controller ready to start integration of next image");
    }
      
    /* callback hit; a new image is ready to be handled */
    if(sPCI.image_ready){

#ifdef DEBUG_POLONATORACQ
      sprintf(log_string, "STATUS:\tPolonator-acquirer: started handling image %d", sPCI.num_imgs-1);
      p_log(log_string);
      if(sPCI.num_imgs==1) p_log("STATUS:\tPolonator-acquirer: first image received from camera...");
#endif

      /* Once in a while, the readout_started event is missed by the framegrabber; if this happens,
	 execute the code that this would normally trigger before executing the image_ready code
      if(caught_readout){
	caught_readout=0;
      }
      else{
	p_log("ERROR: IMAGE_READY event occurred without a READOUT_STARTED event; handling the 'unexpected' image");
	if((sPCI.num_imgs)&&(num_to_average==0)){
	  p_log("STATUS:\tPolonator-acquirer: wait for processor ready to receive image");
	  if(network_waitforsend(clnt_sock)!=1){
	    p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
	    exit_on_error(m_sock, hCamera, serv_sock);
	  }
	}
	p_log("STATUS:\tPolonator-acquirer: processor ready to receive image");
	p_log("STATUS:\tPolonator-acquirer: signalling controller ready to start integration of next image");
	
	p_log("STATUS:\tPolonator-acquirer: finished signalling controller ready to start integration of next image");
      }

      */

      
      /* record current time so we know how long until the next image */
      gettimeofday(&tv2, NULL);

      /* get pointer to image so we can manipulate it */
      eStat = PHX_Acquire(hCamera, PHX_BUFFER_GET, &img_buffer);
      check_for_error(eStat, function_name, "PHX_Acquire(PHX_BUFFER_GET)");

      /* if averaging is turned on, do the averaging, then transmit if averaging is complete; */
      /* if averaging is off, transmit the image */
      if(num_to_average){
	/* add current buffer to the average */
	p_log("add current image to average");
	average_images(img_buffer.pvAddress);

	if(averaging_complete){
	  if(!first_average_image){
	    p_log("STATUS:\tPolonator-acquirer: averaging complete; wait for processor ready to receive image");
	    while(network_waitforsend(clnt_sock)!=1){
	      p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
	      exit_on_error(m_sock, hCamera, serv_sock);
	    }
	  }
	  else{
	    first_average_image = 0;
	  }
	  p_log("STATUS:\tPolonator-acquirer: processor ready to receive image");

	  /* transmit the image; acquisition of next image has already started */
	  p_log("STATUS:\tPolonator-acquirer: sending image to processor");
	  network_sendimage(clnt_sock, fcnum, curr_array, curr_img, average_image);
	  p_log("STATUS:\tPolonator-acquirer: finished sending image to processor");
	}

      }
      else{

	/* transmit the image; acquisition of next image has already started */
	p_log("STATUS:\tPolonator-acquirer: sending image to processor");
	network_sendimage(clnt_sock, fcnum, curr_array, curr_img, img_buffer.pvAddress);
	p_log("STATUS:\tPolonator-acquirer: finished sending image to processor");

      }

      /* release buffer back to Phoenix's buffer pool */
      PHX_Acquire(hCamera, PHX_BUFFER_RELEASE, &img_buffer);

      /* reset flag so callback knows the image is finished being handled */
      p_log("STATUS:\tPolonator-acquirer: reset callback flag");
      sPCI.image_ready = 0;
 
      /* changed on 100609*/
      maestro_setflag(m_sock);

      /* update image, array, fc counters */
      if((!num_to_average) || (averaging_complete)){
	sprintf(log_string, "Finished handling\t%d\t%d\t%d\t%d", fcnum, curr_array, curr_img, sPCI.num_imgs);
	p_log_simple(log_string);
	curr_img++;
	if(curr_img == TOTAL_IMGS){ /* ready to increment array counter, reset image counter */
	  curr_img = 0;
	  curr_array++;
	  fprintf(stdout, "Finished handling\t  \t  \t    \t            \n");
	  fflush(stdout);
	  if(curr_array == TOTAL_ARRAYS){ /* ready to increment flowcell counter, reset array counter */
	    curr_array = 0;
	    curr_fc++;
	    fprintf(stdout, "Finished handling\t  \t  \t    \t          \n");
	    fflush(stdout);
	  }
	}
      }
      p_log("STATUS:\tPolonator-acquirer: re-entering image-receive loop...");      
    }
  }
  fprintf(stdout, "\n");
  p_log("STATUS:\tPolonator-acquirer: last image received from camera...");
  maestro_resetflag(m_sock);
  
  p_log("STATUS:\tPolonator-acquirer: release camera handle...");
  if(hCamera) PHX_CameraRelease(&hCamera);
  network_waitforsend(clnt_sock);  /*don't close connection until last image is transmitted */
  shutdown(serv_sock, 2);
  shutdown(m_sock, 2);
  return 0;
}

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

  /* event PHX_INTRPT_BUFFER_READY evaluates true when the framegrabber receives a
     full image from the camera; at this point, we can get a pointer to the image
     in the framegrabber's memory pool and copy it out for our own use
  */
  if(PHX_INTRPT_BUFFER_READY & dwInterruptMask){

    /* callback sets image_ready to true on execution, then main thread */
    /* resets it to false when it is finished with the image; therefore,*/
    /* it should always be false when the callback is executed */
    /* if it is not, it means code in the main thread (Polonator-acquirer.c) 
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

  /* event PHX_INTRPT_FRAME_START evaluates true when the framegrabber starts
     receiving a new image from the camera; we cannot get the image yet, but
     our software uses this to synchronize with the controller in step-and-
     settle mode
  */
/*  else if(PHX_INTRPT_FRAME_START & dwInterruptMask){
    sPCI->readout_started = 1;

#ifdef DEBUG_POLONATORACQ
    p_log("PHX CALLBACK: READOUT STARTED");
#endif

  }
*/
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
  /* open the port, and wait for processor to connect 
  network_startserver(&serv_sock, &clnt_sock, proc_portnum);*/
  
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

int send_FL_images(char *mystring, int num_array, int clnt_sock)
{
  /* Network transfer declarations */
  int serv_sock; /* socket for the server (this machine) */
  int i = 0;
  int j = 0;
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

  sprintf(log_string, "string1 length %d", strlen(string1));
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
  sprintf(log_string, "success_%d.raw", 0);
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

  sprintf(stagealign_rawimgfilename,"%simage_%d.raw",stagealign_dir_name,(60+i*10));

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
  sprintf(log_string,"Trying to send autoexposure FL images %d", i);
  p_log_simple(log_string);
  network_sendimage(clnt_sock, 0, 0, i, blank_image);
  sprintf(log_string,"Sent autoexposure FL images %d", i);
  p_log_simple(log_string);
i++;
  
}

i =0;

  while(i<2)
  {
		i = network_waitforsend(clnt_sock);
		sprintf(log_string,"waiting to receive the gain value back");
		p_log(log_string);
  }

  gain = (i-97)*10+60;

  sprintf(log_string,"the autoexposure gain to use will be %d", gain);
  p_log_simple(log_string);
  
  return gain;

}	


