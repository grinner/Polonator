
/* =============================================================================
// 
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Polonator_maestrofunctions.c: functionality for communicating with the
// Maestro controller
//
// Release 1.0 -- 04-15-2008
// Release 2.0 -- 12-16-2008 Modified for PolonatorScan controller software [GP]
// Release 3.0 -- 11-24-2009 Modified for PolonatorScan controller software [NC]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "get_sock.h"
#include "logger.h"
#include "config.h"
#include "maestro_functions.h"

#include "global_parameters.h" // added by NC 11/24/2009


#define WAIT usleep(1000)

char log_string[LOG_STR_BUF0];


/* Python SWIG interface function which sets a global variable
 */
void py_maestro_open(char *machine_name, int port){
    py_sock = GetSock(machine_name, port);
    maestro_open(&py_sock);
    start_logger(LOG_FILE_FULL_PATH,1);
}


/* Open a Telnet connection to the Maestro controller and return the socket */
/* as m_sock to be used for subsequent communication with the device */
void maestro_open(int *m_sock)
{
  
    /* control messages to start Telnet session */
    unsigned char a0[] = {255,254,1};
    unsigned char a01[] = {255,254,3};

    int bytes_sent;

    /* open connection to Maestro on the Telnet port (23) */
    /* by default, the controller IP is associated with hostname */
    /* "controller" in /etc/hosts */
    *m_sock = GetSock("controller", 23);

    /* the Maestro initates communication w/ one 3-byte message */
    /* all 'control' messages from Maestro are discarded since our */
    /* 'client's' behavior is hard-coded */
    consume_bytes(*m_sock, 3);

    /* send first control message to Maestro */
    bytes_sent = send(*m_sock, a0, 3, 0);
    sprintf(log_string, "STATUS:\tPolonator_maestrofunctions: Sent %d bytes", bytes_sent);
    p_log_simple(log_string);

    /* we expect a 3-byte response */
    consume_bytes(*m_sock, 3);

    /* send second control message*/
    bytes_sent = send(*m_sock, a01, 3, 0);
    sprintf(log_string, "STATUS:\tPolonator_maestrofunctions: Sent %d bytes", bytes_sent);
    p_log_simple(log_string);

    /* Maestro prints a 'welcome' screen; discard this */
    consume_bytes(*m_sock, 58);
    fprintf(stdout, "consumedbytes\n");
    /* Commands can now be issued to the Maestro; append \n\r to */
    /* the end of all commands; Maestro appends >\n\r to the end of */
    /* all responses; these should be discarded also */
}

/* Used to gracefully 'reset' the software on the Maestro if a scan
   has been stopped while running (e.g. by a software crash, etc).  When
   this happens, the maestro code thinks everything is just 'paused', but
   the acquisition computer thinks everything is 'done'.  So we need to
   tell the maestro software that it should quickly exit the scan routine
   to be ready for the next scan
*/
void maestro_stop(int m_sock)
{
    char response[RESPONSE_BUF1];
    int response_length;
    char command[CMD_BUF1];

    sprintf(command, "PolonatorScan.INTERRUPT_SCAN=1\n\r");

    /* First, stop hitting the array with light by moving the filter
        out of the way
    */
    maestro_setcolor(m_sock, "none");

    /* INTERRUPT_SCAN flag on controller is used to break out of the scan
        loops; we still need to move through the current set of X moves
        since the status of INTERRUPT_SCAN is monitored in PolonatorScan.run,
        not the X code (which runs asynchronously)
    */
    p_log_simple("Setting stop flag on controller...");
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    /* now finish the current set of X moves; we'll know we're done when
        global variable PolonatorScan.STATUS goes to 3 -- this is set
        to 4 when the scanning routine is running, and 3 when it is not
    */
    p_log_simple("Running through the current set of x moves...");
    while(maestro_get_scan_status(m_sock) > 3)
    {
        p_log_simple("STATUS:\tmaestro_setflag: waiting to set flag on controller to start trigger");
        maestro_setflag(m_sock);
        usleep(5*SLEEP_CYCLES);
    }
    maestro_resetflag(m_sock);
}
  
/* Used by maestro_stop to determine when the current set of X moves
   has completed
*/
int maestro_get_scan_status(int m_sock)
{
    char response[RESPONSE_BUF1];
    int response_length;
    char command[CMD_BUF1];

    sprintf(command, "PolonatorScan.STATUS\n\r");

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    return response[0]-48;
}

/* Used by Polonator-acquirer during the scan routine to
   unset controller 'pause flag' (set the flag to 'ON');
   the flag is global variable iprdy on the X axis, and tells
   the X axis code that the acquisition computer is ready for
   the next image to be acquired
*/
void maestro_setflag(int m_sock)
{
    unsigned char command[] = "x.iprdy=1\n\r";
    char response[RESPONSE_BUF1];
    int response_length;
    int a;

    p_log("STATUS:\tmaestro_setflag: set flag on controller to start trigger");
    a = send(m_sock, command, 11, 0);
    if(a < 11)
    {
        p_log("ERROR: NOT ENOUGH BYTES SENT FOR SETFLAG");
    }
    p_log("STATUS:\tmaestro_setflag: wait for response from controller");
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);
}


/* set controller pause flag so acquisition stops; normally the controller
   handles this
*/
void maestro_resetflag(int m_sock)
{
    char command[] = "x.iprdy=0\n\r";
    char response[100];
    int response_length;

    p_log("STATUS:\tmaestro_resetflag: set flag on controller to stop");
    send(m_sock, command, strlen(command), 0);
    p_log_simple("STATUS:\tmaestro_resetflag: wait for response from controller");
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}
  

/* returns the pause flag value */
int maestro_getflag(int m_sock)
{
    char input[5];
    char command[] = "x.iprdy\n\r";
    int bytes_received;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, input, &bytes_received);
    return atoi(input);
}


/* parses a response from the maestro; assumes all responses end with the
   > character, and has a limit of INPUT_BUF0 chars for the response length; response
   is returned as a null-terminated string into array response, and its length
   (not including the null terminator) is returned in response_length
*/
void maestro_readresponse(int m_sock, char *response, int *response_length)
{
    char input_buffer[INPUT_BUF0]; // looks like this is unecessary NC 11/09/2010
    int i;
    int bytes_received = 0;
    *response_length = 0;

    while(bytes_received < INPUT_BUF0)
    {
        bytes_received = recv(m_sock, response+bytes_received, INPUT_BUF0-bytes_received, 0);
        for(i = 0; i < bytes_received; i++)
        {

#ifdef DEBUG_MAESTRO
            sprintf(log_string, "maestro sent byte %d: %c", i, *(response + i));
            p_log(log_string);
#endif

            if(*(response+i) != '>')
            {
        	    *response_length = *response_length + 1;
            }
            else
            {
                *response_length = *response_length - 2;
                *(response + *response_length) = '\0';
                bytes_received = INPUT_BUF0;
                i = bytes_received;
            }
        } // end for
    WAIT;
    } // end while
}

void maestro_readresponse2(int m_sock, char *response, int *response_length)
{
    //char input_buffer[INPUT_BUF0]; // looks like this is unecessary NC 11/09/2010
    int i;
    int bytes_received = 0;
    int limit = *response_length; // added NC 11/09/2010
    *response_length = 0;

    // while(bytes_received < INPUT_BUF0)
    while(bytes_received < limit) // added NC 11/09/2010
    {
        bytes_received = recv(m_sock, response+bytes_received, INPUT_BUF0-bytes_received, 0);
        for(i = 0; i < bytes_received; i++)
        {

#ifdef DEBUG_MAESTRO
            sprintf(log_string, "maestro sent byte %d: %c", i, *(response + i));
            p_log(log_string);
#endif
            if(*(response+i) != '>')
            {
        	    *response_length = *response_length + 1;
            }
            else
            {
        	    *response_length = *response_length - 2;
        	    *(response + *response_length) = '\0';
        	    //bytes_received = INPUT_BUF0;
        	    bytes_received = limit; // added NC 11/09/2010
        	    i = bytes_received;
            }
        } // end for
        WAIT;
    } // end while
}

/* used to move stage to one of the alignment positions -- there is one per
   lane, and the position is held in the cLANE_ORIGINS_X/Y arrays
*/
void maestro_gotostagealign_position(int m_sock, int flowcell, int lane)
{
    char command[CMD_BUF0];
    char log_string[LOG_BUF0];
    char config_value[CFG_BUF0];
    char response[RESPONSE_BUF0];
    int response_length;
    int lane_index;

    config_open(CONFIG_FILE_FULL_PATH);
    if(!config_getvalue("stagealign_wells_per_fc", config_value))
    {
        p_log_simple("ERROR:\tPolonator-stagealign: config_getval(key stagealign_wells_per_fc) returned no value");
        exit(0);
    }
    lane_index = (atoi(config_value) * flowcell) + lane;
    config_close();

    sprintf(log_string, "Moving stage to alignment position at flowcell %d, lane %d...", flowcell, lane);
    p_log_simple(log_string);

    p_log_simple("Command X move to position:");
    sprintf(command, "X.pa=PolonatorScan.cLANE_ORIGINS_X[%d]+PolonatorScan.OFFSET_X[%d]; X.bg\n\r", lane_index, lane_index);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Command Y move to position:");
    sprintf(command, "Y.pa=PolonatorScan.cLANE_ORIGINS_Y[%d]+PolonatorScan.OFFSET_Y[%d]; Y.bg\n\r", lane_index, lane_index);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Wait for stage to reach position...");
    while(maestro_stageinmotion(m_sock))
    {
        usleep(SLEEP_CYCLES);
    }

    p_log_simple("Actual X position after move:");
    sprintf(command, "X.px\n\r");
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Actual Y position after move:");
    sprintf(command, "Y.px\n\r");
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

/* Used to move stage to an image position relative to the offset origin
 * An alignment step is separate from this move.
 *
 * added by NC 11/22/09
 *
*/
void py_maestro_goto_image(int flowcell, int lane, int image_number)
{
  maestro_goto_image(py_sock, flowcell, lane, image_number);
}
void maestro_goto_image(int m_sock, int flowcell, int lane, int image_number)
{
	char command[CMD_BUF0];
	char log_string[LOG_BUF0];
	char config_value[CFG_BUF0];
	char response[RESPONSE_BUF0];
	int response_length;
	int lane_index;
	int image_LLcornerX, image_LLcornerY; /* the lower left hand corner of an image on... */
	/* a grid of pixels covering the entire flowcell */

	/* Hold offsets found by alignment in pixels and stageunits */
	double goto_x, goto_y,center_x,center_y;

	/* Image acquisition settings, populated from config file */
	/*int stagealign_integration_inmsec; */
	/*int stagealign_gain; */
	/*int stagealign_well; */
	int stagealign_wells_per_fc;

	/* Values used for conversion between pixels and stage units */
	int stagealign_optical_mag;
	int ccd_pixel_size;
	double pixelsize_at_stage; /* size of a pixel at the stage in microns */

	/* Holds previous offsets from controller in case the alignment doesn't work */
	int curr_offset_x, curr_offset_y;

	/* Holds encoder resolutions from controller, used for calculating distance of move */
	double encoder_res_X, encoder_res_Y;



	/* CONVERT FROM Images to pixels */

	if (image_number < TOTAL_IMG_PER_LANE)
	{
		/* Y runs the width of the flow cell and X runs the length of the flow cell */
		image_LLcornerY = (image_number / TOTAL_IMG_PER_X_SCAN_LINE)*IMAGE_RESOLUTION_Y;
		image_LLcornerX = ((image_number % TOTAL_IMG_PER_X_SCAN_LINE))*IMAGE_RESOLUTION_X;
	}
	else
	{

	}

	/* make like a stage align */

	config_open(CONFIG_FILE_FULL_PATH);

	if(!config_getvalue("stagealign_optical_mag", config_value))
	{
		p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_optical_mag) returned no value");
		exit(0);
	}
	stagealign_optical_mag = atoi(config_value);

	if(!config_getvalue("stagealign_ccd_pixel_size", config_value))
	{
		p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_ccd_pixel_size) returned no value");
		exit(0);
	}
	ccd_pixel_size = atoi(config_value);

	if(!config_getvalue("stagealign_wells_per_fc", config_value))
	{
		p_log_simple("ERROR:\tPolonator-stagealign: config_getval(key stagealign_wells_per_fc) returned no value");
		exit(0);
	}
	stagealign_wells_per_fc = atoi(config_value);
	lane_index = (atoi(config_value) * flowcell) + lane;

	config_close();

	/* LOAD ENCODER RESOLUTIONS FOR CONVERSION BELOW; THESE ARE STORED ON */
	/* THE CONTROLLER AS COUNTS PER MILLIMETER */
	p_log("Retrieving encoder resolutions...");
	sprintf(command, "PolonatorScan.cENCODER_X_RESOLUTION\n\r");
	p_log(command);
	send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log(response);
	encoder_res_X = atof(response);

	sprintf(command, "PolonatorScan.cENCODER_Y_RESOLUTION\n\r");
	p_log(command);
	send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log(response);
	encoder_res_Y = atof(response);

	/* CONVERT FROM PIXELS TO STAGE UNITS */
	/* CALCULATE PIXEL SIZE IN MILLIMTERS AT THE STAGE BASED */
	/* ON THE MAGNIFICATION AND THE CCD PIXEL SIZE */
	pixelsize_at_stage = ((double)ccd_pixel_size / (double)stagealign_optical_mag) / IMAGE_RESOLUTION_X;

	/* Does the "*-1" mean the controls are inverted? */
	center_x = (double) IMAGE_RESOLUTION_X*TOTAL_IMG_PER_X_SCAN_LINE/2*pixelsize_at_stage * encoder_res_X;
    center_y = (double) IMAGE_RESOLUTION_Y*TOTAL_IMG_PER_Y_SCAN_LINE/2*pixelsize_at_stage * encoder_res_Y;
    goto_x = (double) image_LLcornerX * pixelsize_at_stage * encoder_res_X - center_x;
	goto_y = (double) image_LLcornerY * pixelsize_at_stage * encoder_res_Y;// + center_y;


	sprintf(log_string, "Moving stage to alignment position at flowcell %d, lane %d...", flowcell, lane);
	p_log_simple(log_string);

	p_log_simple("Command X move to position:"); //cCENTER_XcLANE_ORIGINS_X
	sprintf(command, "X.pa=PolonatorScan.cCENTER_X[%d]+PolonatorScan.OFFSET_X[%d]+%d; X.bg\n\r", flowcell, lane_index,(int) goto_x);
	//sprintf(command, "X.pa=22015561; X.bg\n\r");
        send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log_simple(response);

	p_log_simple("Command Y move to position:"); //cLANE_ORIGINS_YcCENTER_Y
    //goto_y = image_number;
    if (flowcell != 0)
    {
        goto_y = 0;
        sprintf(command, "Y.pa=PolonatorScan.cLANE_ORIGINS_Y[%d]+250000+PolonatorScan.OFFSET_Y[%d]+%d; Y.bg\n\r", lane_index, lane_index, (int) goto_y);
        //sprintf(command, "Y.pa=PolonatorScan.cCENTER_Y[%d]+250000+PolonatorScan.OFFSET_Y[%d]+%d; Y.bg\n\r", flowcell, lane_index, (int) goto_y);
        //sprintf(command, "Y.pa=PolonatorScan.cCENTER_Y[%d]; Y.bg\n\r", lane_index);
    }
    else
    {
        //-100000
        goto_y = 0;
        sprintf(command, "Y.pa=PolonatorScan.cLANE_ORIGINS_Y[%d]+PolonatorScan.OFFSET_Y[%d]+%d; Y.bg\n\r", lane_index, lane_index, (int) goto_y);
    }
    send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log_simple(response);

	p_log_simple("Wait for stage to reach position...");
	while(maestro_stageinmotion(m_sock))
	{
		usleep(SLEEP_CYCLES);
	}

	p_log_simple("Actual X position after move:");
	sprintf(command, "X.px\n\r");
	send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log_simple(response);

	p_log_simple("Actual Y position after move:");
	sprintf(command, "Y.px\n\r");
	send(m_sock, command, strlen(command), 0);
	maestro_readresponse(m_sock, response, &response_length);
	p_log_simple(response);

}


/* used to 'start' the controller to do a single set of imaging (one
   color); sets fluor, integration time in milliseconds, and number 
   of images to average (if darkfield imaging);
   includes python SWIG interface (where the controller socket is maintained
   as a global variable)
*/
void py_maestro_setupimaging(int filter, int integration_time, int num_imgs, int num_lanes, int fcnum, int shutter_flag, int TDI_flag)
{
    maestro_setupimaging(py_sock, filter, integration_time, num_imgs, num_lanes, fcnum, shutter_flag, TDI_flag);
}
void maestro_setupimaging(int m_sock, int filter, int integration_time, int num_imgs, int num_lanes, int fcnum, int shutter_flag, int TDI_flag)
{
    char command[CMD_BUF1];
    char log_string[LOG_BUF1];
    char response[RESPONSE_BUF1];
    int response_length;

    /* SET TDI FLAG (0==STEP/SETTLE, 1==TDI) */
    sprintf(command, "PolonatorScan.TDI_FLAG=%d\n\r", TDI_flag);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting TDI flag to <%d>", TDI_flag);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);  


    /* SET SHUTTER STATE (0==INACTIVE, 1==ACTIVE) */
    sprintf(command, "PolonatorScan.SHUTTER_FLAG=%d\n\r", shutter_flag);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting shutter flag to <%d>", shutter_flag);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);  


    /* SET INTEGRATION TIME */
    sprintf(command, "PolonatorScan.INTEGRATION_TIME=%d\n\r", integration_time);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting integration time of <%d> milliseconds on controller (+5)", integration_time);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);


    /* SET NUMBER OF IMAGES PER POSITION */
    sprintf(command, "PolonatorScan.NUM_IMGS=%d\n\r", num_imgs);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting number of images per position to <%d>", num_imgs);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);


    /* SET FILTER */
    sprintf(command, "PolonatorScan.FILTER=%d\n\r", filter);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting filter to <%d>", filter);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);


    /* SET NUMBER OF LANES TO IMAGE (STARTING WITH LANE 0) */
    sprintf(command, "PolonatorScan.NUM_LANES=%d\n\r", num_lanes);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting number of lanes to <%d>", num_lanes);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);


    /* SET FLOWCELL TO IMAGE (0==LEFT, 1==RIGHT) */
    sprintf(command, "PolonatorScan.FLOWCELL=%d\n\r", fcnum);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting flowcell to <%d>", fcnum);
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);

    /* SET START_SCAN */
    sprintf(command, "PolonatorScan.START_SCAN=1\n\r");
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Setting START_SCAN to 1");
    p_log_simple(log_string);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    sprintf(log_string, "STATUS:\tmaestro_setupimage: Controller responded with <%s>", response);
    p_log_simple(log_string);

    /* send strings for starting and ending wells; should be [0..35];
        usually, if imaging one full flowcell, it is 0,17 or 18,35
    */
}


/* Sets the current FLOATING POINT focus offset; this varies from one machine
   to another, and is specifies as an integer value between 0 and 40, which is
   mapped onto the real interval -1..1
*/
void maestro_setfocus(int m_sock, int focus)
{
    char command[CMD_BUF0];
    char response[RESPONSE_BUF0];
    char log_string[LOG_BUF0];
    int response_length;
    float flfocus;

    flfocus = (((float)focus)/FOCUS_OFFSET_LIM)-1; /* not sure about this divide by 40 business */

    sprintf(command, "z.as[1]=%0.02f\n\r", flfocus);
    p_log_simple(command);
    send(m_sock, command, strlen(command), 0); 
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}


/* Commits the current focus to non-volatile memory
 */
void maestro_writefocus(int m_sock)
{
    char command[CMD_BUF0];
    char response[RESPONSE_BUF0];
    int response_length;

    strcpy(command, "z.kl\n\r");
    p_log_simple(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    strcpy(command, "z.sv\n\r");
    p_log_simple(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    strcpy(command, "z.xq##mh\n\r");
    p_log_simple(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    sleep(2);
}


/* 
Returns the current INTEGER value of the focus offset; this will vary from one
   machine to another, and is a real number.  We assume the number will always
   be in the range [-1..1], and allow intervals of 0.05.  Therefore, we scale
   the value returned by adding 1 and multiplying by 20, for possible focus offset
   values of [0..40].
*/
int maestro_getfocus(int m_sock)
{
    char command[] = "z.as[1]\n\r";
    char response[RESPONSE_BUF0];
    int response_length;
    float focus;
    char strfocus[FOCUS_BUF0];
    int intfocus;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
    focus = atof(response);
    intfocus = (int)((focus + 1)*FOCUS_OFFSET_LIM);
    return intfocus;
}


/* Used to set the fluorescence filter wheel to the desired filter block;
   char *color can be "fam", "cy5", "cy3", "txred", or "none"
*/
void py_maestro_setcolor(char *color)
{
    maestro_setcolor(py_sock, color);
}

void maestro_setcolor(int m_sock, char *color)
{
    char filter_number;
    char command[CMD_BUF2];
    char response[RESPONSE_BUF2];
    int response_length;
    char log_string[LOG_BUF2];

    /* 
    determine the actual position on the wheel to move to; 
    the mapping from 'color' to 'color number' is determined 
    by Danaher when they build the instrument (e.g. "fam" 
    is position 1
    */

    if(strcmp(color, "fam") == 0)
    {
        filter_number = '1';
    }
    else if(strcmp(color, "cy5") == 0)
    {
        filter_number = '3';
    }
    else if(strcmp(color, "cy3") == 0)
    {
        filter_number = '0';
    }
    else if(strcmp(color, "txred") == 0)
    {
        filter_number = '2';
    }
    else if(strcmp(color, "spare") == 0)
    {
        filter_number = '5';
    }
    else
    {
        filter_number = '4';
    }

    /*RCT added theta.done=0 otherwise theta.done=1 always*/
    send(m_sock, "Theta.done=0\n\r", 15, 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    sprintf(command, "Theta.xq##gotofilter(%d);\n\r", filter_number-48);
    p_log_simple(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);


    p_log("Wait for motion to complete");
    /*RCT Logic for done is 1 when complete changed while(maestro_thetainmotion(m_sock)) to while(maestro_thetainmotion(m_sock)==0)*/
    while(maestro_thetadone(m_sock) == 0)
    {
        usleep(SLEEP_CYCLES);
    }
}


/* Used by maestro_setcolor to determine whether the filter axis (theta) has
   reached its commanded position or is still in motion; returns 1 if motion
   is complete, or 0 otherwise
*/
int maestro_thetadone(int m_sock)
{
    char command[CMD_BUF2];
    char response[RESPONSE_BUF2];
    int response_length;

    sprintf(command, "Theta.done\n\r");
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    if(response[0] == '1')
    {
        return 1;
    }
    else
    {
        usleep(100000);
        return 0;
    }
}


/* Returns 1 if either X or Y is in motion (ms != 0), 0 if
   both are not in motion
 */
int maestro_stageinmotion(int m_sock)
{
    char input1[5];
    char command1[] = "x.ms\n\r";
    char command2[] = "y.ms\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);

    if(response[0] == '0')
    {
        send(m_sock, command2, strlen(command2), 0);
        maestro_readresponse(m_sock, response, &response_length);

        if(response[0] == '0')
        {
            return 0;
        }
    }
    return 1;
}



void maestro_unlocktheta(int m_sock)
{
    char command[] = "Theta.mo=0\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

void maestro_locktheta(int m_sock)
{
    char command[] = "Theta.mo=1\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}


/* Used to home the theta axis after it has been manually unlocked
 */
void maestro_hometheta(int m_sock)
{
    char command1[] = "Theta.hmstat=0\n\r";
    char command2[] = "Theta.xq##mh\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);

    p_log("Send Theta home command");
    send(m_sock, command2, strlen(command2), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log("Wait for Theta homing to complete");
    while(maestro_thetahoming(m_sock))
    {
        usleep(SLEEP_CYCLES);
    }
}

void py_maestro_hometheta(void)
{
    maestro_hometheta(py_sock);
}

void py_maestro_unlocktheta(void)
{
    maestro_unlocktheta(py_sock);
}

void py_maestro_locktheta(void)
{
    maestro_locktheta(py_sock);
}

/* RCT Used by maestro_hometheta to determine whether the filter axis (theta) has
   completed the homing process or is still in homing; returns 1 if homing
   is still executing, or 0 if complete
*/
int maestro_thetahoming(int m_sock)
{
    char command[] = "Theta.hmstat\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);

    if(response[0] == '1')
    {
        return 0;
    }

    return 1;
}



/* Called to change TDI status; 1==TDI, 0==step/settle
 */
void maestro_setTDI(int m_sock, int tdi_flag)
{
    char command[100];
    char response[100];
    int response_length;

    sprintf(command, "PolonatorScan.TDI=%d\n\r", tdi_flag);
    p_log("Set TDI flag on controller");
    p_log(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);
}



/* Called to set motion controller for WL imaging
 */
void maestro_setWL(int m_sock)
{
    char command[] = "PolonatorScan.SHUTTER_FLAG=1\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    p_log("Setup controller for WL imaging");

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);
}

/* Used to unlock the X and Y axes to manually move the stage; this should
   not be done during a run, as the absolute positions can be lost and result
   in image alignment failure
 */
void py_maestro_unlock(void)
{
    maestro_unlock(py_sock);
}

void maestro_unlock(int m_sock)
{
    char command1[] = "x.mo=0\n\r";
    char command2[] = "y.mo=0\n\r";

    char response[RESPONSE_BUF1];
    int response_length;

    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    send(m_sock, command2, strlen(command2), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

/* Used to lock the X and Y axes after they have been unlocked
 */
void py_maestro_lock(void)
{
    maestro_lock(py_sock);
}

void maestro_lock(int m_sock)
{
    char command1[] = "x.mo=1\n\r";
    char command2[] = "y.mo=1\n\r";

    char response[RESPONSE_BUF1];
    int response_length;

    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);

    send(m_sock, command2, strlen(command2), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);
}

void maestro_getstatus(int m_sock)
{
    char command0[] = "PolonatorScan.STATUS\n\r";
    char command1[] = "list -p:r\n\r";
    char command2[] = "list -p:a\n\r";
    char command3[] = "error\n\r";
    char command4[] = "x.error\n\r";
    char command5[] = "y.error\n\r";
    char command6[] = "z.error\n\r";
    char command7[] = "theta.error\n\r";
    char command8[] = "shutter.error\n\r";
    char command9[] = "x.iprdy\n\r";
    char command10[] = "list\n\r";
    char command11[] = "PolonatorScan.START_SCAN\n\r";

    char response[RESPONSE_BUF3];
    int response_length;

    p_log_simple("PolonatorScan status:");
    send(m_sock, command0, strlen(command0), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Running processes:");
    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Aborted processes:");
    send(m_sock, command2, strlen(command2), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Current Maestro error (OK means 'none'):");
    send(m_sock, command3, strlen(command3), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log("Current X-axis error (OK means 'none'):");
    send(m_sock, command4, strlen(command4), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);

    p_log_simple("Current Y-axis error (OK means 'none'):");
    send(m_sock, command5, strlen(command5), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Current Z-axis error (OK means 'none'):");
    send(m_sock, command6, strlen(command6), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Current theta-axis error (OK means 'none'):");
    send(m_sock, command7, strlen(command7), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Current shutter-axis error (OK means 'none'):");
    send(m_sock, command8, strlen(command8), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Acquire flag status (x.iprdy) -- should never be '1' if controller is idle:");
    send(m_sock, command9, strlen(command9), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("Scan flag status (PolonatorScan.START_SCAN) -- should only be '1' if the Polonator is actively scanning:");
    send(m_sock, command11, strlen(command11), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);

    p_log_simple("Connected axes -- should list X, Y, Z, theta, and shutter:");
    send(m_sock, command10, strlen(command10), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

void maestro_debug(int m_sock)
{
    char command12[] = "x.iprdy\n\r";
    char response[RESPONSE_BUF3];
    int response_length;

    p_log("x.iprdy:");
    send(m_sock, command12, strlen(command12), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

void py_maestro_darkfield_off(void)
{
    maestro_darkfield_off(py_sock);
}



void maestro_darkfield_off(int m_sock)
{
    char command[] = "theta.ob[2]=0\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

void py_maestro_darkfield_on(void)
{
    maestro_darkfield_on(py_sock);
}

void maestro_darkfield_on(int m_sock)
{
    char command[] = "theta.ob[2]=1\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
}

void maestro_autofocus_on(int m_sock)
{
    char command[] = "z.xq##autofocus()\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    p_log_simple("Autofocus on...");
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
    sleep(1);
}

void py_maestro_shutter_open(void)
{
    maestro_shutteropen(py_sock);
}

void maestro_shutteropen(int m_sock)
{
    char command[] = "x.ob[2]=1\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    p_log_simple("Shutter open...");

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
    usleep(SLEEP_CYCLES);
}

void py_maestro_shutter_close(void)
{
    maestro_shutterclose(py_sock);
}

void maestro_shutterclose(int m_sock)
{
    char command[] = "x.ob[2]=0\n\r";
    char response[RESPONSE_BUF1];
    int response_length;

    p_log_simple("Shutter close...");

    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);
    usleep(10000);
}

/* used to stop all programs running on the controller, reset it,
   re-home all axes, and re-initialize all controller-resident code;
   this should NOT be executed during a run as the absolute positions 
   can be lost and result in image alignment failure
*/
void maestro_reset(int m_sock)
{
    char command1[] = "kill\n\r";
    char command2[] = "restart\n\r";
    char command3[] = "restarta\n\r";
    char command4[] = "x.hmstat=0\n\r";
    char command5[] = "y.hmstat=0\n\r";
    char response[RESPONSE_BUF0];
    int response_length;


    p_log_simple("STATUS:\tmaestro_reset: Kill running programs on controller; response:");
    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("STATUS:\tmaestro_reset: Reset controller; response:");
    send(m_sock, command2, strlen(command2), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    send(m_sock, command4, strlen(command4), 0);
    maestro_readresponse(m_sock, response, &response_length);
    send(m_sock, command5, strlen(command5), 0);
    maestro_readresponse(m_sock, response, &response_length);

    p_log_simple("STATUS:\tmaestro_reset: Execute controller startup routine; response:");
    send(m_sock, command3, strlen(command3), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log_simple(response);

    p_log_simple("STATUS:\tmaestro_reset: Wait for homing to complete");
    while(maestro_homing(m_sock))
    {
        sleep(1);
    }
}


/* 
returns 1 if X and/or Y is currently homing, 0 if both are homed (x.hmstat
and y.hmstat == 1)
*/
int maestro_homing(int m_sock)
{
    char command1[] = "x.hmstat\n\r";
    char command2[] = "y.hmstat\n\r";
    char response[RESPONSE_BUF0];
    int response_length;

    /* X axis */
    send(m_sock, command1, strlen(command1), 0);
    maestro_readresponse(m_sock, response, &response_length);

    if(response[0] == '1')
    {
        /* Y axis */
        send(m_sock, command2, strlen(command2), 0);
        maestro_readresponse(m_sock, response, &response_length);
        if(response[0] == '1')
        {
            return 0; /* X and Y are homed */
        }
    }
    return 1; /* either X or Y or both not homed */
}
  
/* used to snap a single image */
void maestro_snap(int m_sock, int integration_inmsec, int shutterflag)
{
    char command[CMD_BUF0];
    char response[RESPONSE_BUF0];
    int response_length;

    if(integration_inmsec < 1)
    {
        integration_inmsec = 1;
    }
    if(integration_inmsec > INTEGRATION_IN_MS_THRESHOLD)
    {
        integration_inmsec = INTEGRATION_IN_MS_THRESHOLD;
    }
    if((shutterflag < 0) || (shutterflag > 1))
    {
        shutterflag = 0;
    }

    p_log_simple("STATUS:\tmaestro_snap(): Execute snap on Maestro:");
    sprintf(command, "x.xq##snap(%d, %d)\n\r", integration_inmsec, shutterflag);
    p_log_simple(command);

    //fprintf(stdout,command); // added NC 11/09/2010 for testing only

    send(m_sock, command, strlen(command), 0);
    response_length = RESPONSE_BUF0; // added NC 11/09/2010
    maestro_readresponse2(m_sock, response, &response_length);
    p_log_simple(response);
}


/* used to 'consume', or ignore, bytes from the input buffer */
void consume_bytes(int m_sock, int num_bytes)
{
    int i;
    int total_bytes_received;
    int current_bytes_received;
    char receiveBuffer[RCV_BUF0];
    char log_string[LOG_BUF0];

    total_bytes_received = 0;
    while(total_bytes_received < num_bytes)
    {
        current_bytes_received = recv(m_sock, (unsigned char*)receiveBuffer+total_bytes_received, num_bytes - total_bytes_received, 0);
        total_bytes_received += current_bytes_received;

#ifdef DEBUG_MAESTRO
        sprintf(log_string,"Consuming %d bytes: %d received: %d:", num_bytes, total_bytes_received, current_bytes_received); 
        p_log(log_string);
        for(i = 0; i < total_bytes_received; i++)
        {
            sprintf(log_string, "maestro sebt byte %d: %c", i, (unsigned char)receiveBuffer[i]);
            p_log(log_string);
        } // end for
#endif
    } // end while
}

