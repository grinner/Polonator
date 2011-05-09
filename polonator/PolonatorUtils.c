/* =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// PolonatorUtils.c: functions for initialization, live imaging, stage control,
// etc.
//
// Release 1.0 -- 04-15-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include "as_phoenix_functions.h"
#include "maestro_functions.h"
#include "network_functions.h"
#include "as_phoenix_live_functions.h"
#include "logger.h"
#include <sys/types.h>

/*#define DEBUG_MAESTRO 1*/
void snap(float exposure, float gain, char *color, char *filename);
static unsigned long long int wait_counter = 0;
static unsigned int attempt_counter = 0;

int main(int argc, char *argv[]){
    int m_sock,i,j, autoexp_gain;
    char autoe_filename[500];
    char autoe_dirname[500];

    if(argc < 2)
    {
        fprintf(stdout, "%s must be called w/ one of the following arguments:\n", argv[0]);
        fprintf(stdout, "\treset\t\t -- restart software on controller and home all axes\n");
        fprintf(stdout, "\tunlock\t\t -- 'unlock' stage for manual movement\n");
        fprintf(stdout, "\tlock\t\t -- 'lock' stage for servo movement\n");
        fprintf(stdout, "\tlive\t\t -- display live darkfield image (%s live <integration in seconds> <EM gain>)\n", argv[0]);
        fprintf(stdout, "\tsnap\t\t -- snap image and save to disk (%s snap <color[fam, cy5, cy3, txred, spare, or none]> <integration in seconds> <EM gain>)\n", argv[0]);
        fprintf(stdout, "\tcolorsnap\t -- snap images in all 4 channels and save to disk (%s colorsnap <integration in seconds> <fam gain> <cy5 gain> <cy3 gain> <txred gain>)\n", argv[0]);
        fprintf(stdout, "\tstatus\t\t -- get motion controller status and last errors, if any\n");
        fprintf(stdout, "\tcomplete-scan\t -- if scanning was interrupted, and reset the controller software gracefully to its starting state\n");
        fprintf(stdout, "\tdarkfield-off\t -- turn the darkfield ring illuminator off\n");
        fprintf(stdout, "\tdarkfield-on\t -- turn the darkfield ring illuminator on\n");
        fprintf(stdout, "\thometheta\t -- 'lock' theta (filter cube) axis and re-home\n");
        fprintf(stdout, "\tunlocktheta\t -- 'unlock' theta (filter cube) axis\n");
        fprintf(stdout, "\tpower-on\t -- turn camera and fluorescence light source on\n");
        fprintf(stdout, "\tpower-off\t -- turn camera and fluorescence light source off\n");
        fprintf(stdout, "\tgotostagealignpos\t -- goto position used for stage alignment (%s gotostagealignpos <flowcell> <lane>)\n", argv[0]);
        fprintf(stdout, "\tgetfocus\t -- get current 'focus' value from Z axis (this is the offset applied to focus error signal)\n");
        fprintf(stdout, "\tsetfocus\t -- set focus value for Z axis; note this DOES NOT write to non-volatile memory (%s setfocus <focus_value>)\n", argv[0]);
        fprintf(stdout, "\twritefocus\t -- write current focus value to non-volatile memory; should be used after correct value is found with setfocus\n");
        exit(1);
    }
    /* Reset the controller software and re-home the stage */
    if(strcmp(argv[1], "reset") == 0){
        maestro_open(&m_sock);
        maestro_reset(m_sock);
    }

    else if(strcmp(argv[1], "unlock") == 0){
        maestro_open(&m_sock);
        maestro_unlock(m_sock);
    }

    else if(strcmp(argv[1], "lock") == 0){
        maestro_open(&m_sock);
        maestro_lock(m_sock);
    }

    else if(strcmp(argv[1], "getfocus") == 0){
        maestro_open(&m_sock);
        return maestro_getfocus(m_sock);
    }

    else if(strcmp(argv[1], "setfocus") == 0){
        maestro_open(&m_sock);
        maestro_setfocus(m_sock, atoi(argv[2]));
    }

    else if(strcmp(argv[1], "writefocus") == 0){
        maestro_open(&m_sock);
        maestro_writefocus(m_sock);
    }

    /* Display a live darkfield image */
    else if((strcmp(argv[1], "live") == 0) && ((argc == 4)||(argc == 5))){
        maestro_open(&m_sock);

        maestro_darkfield_on(m_sock);    
        maestro_setcolor(m_sock, "none");

    if(argc==4){
        camera_live(argc, argv, 0);
    }
    else{
        camera_live(argc, argv, atoi(argv[4]));
    }
    maestro_darkfield_off(m_sock);
    }

    else if((strcmp(argv[1], "live_new") == 0) && ((argc == 5)||(argc == 6)))
    {
        maestro_open(&m_sock);
        maestro_setcolor(m_sock, argv[4]);

        if(strcmp(argv[4],"none")==0){
	        maestro_darkfield_on(m_sock);
	        maestro_shutterclose(m_sock);
        }
        else{
	        maestro_darkfield_off(m_sock);
	        maestro_shutteropen(m_sock);
        }    

        if(argc==5){
            camera_live(argc, argv, 0);
        }
        else{
            camera_live(argc, argv, atoi(argv[5]));
        }
        if(strcmp(argv[4],"none")==0) maestro_darkfield_off(m_sock);
        else{
	        maestro_shutterclose(m_sock);
        }
    }
  
    else if (strcmp(argv[1],"shutter_close")==0)
    {
	    maestro_open(&m_sock);
  	    maestro_shutterclose(m_sock);
    }
    /* Acquire an image at the current stage position */
    else if((strcmp(argv[1], "snap") == 0) && (argc == 5)){
        snap(atof(argv[3]), atof(argv[4]), argv[2], "snap-image.raw");
    }    

    else if((strcmp(argv[1], "snap1") == 0) && (argc == 6))
    {
        system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images");
        system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images/cy3");
        system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images/fam");
        system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images/cy5");
        system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images/txred");

        /*  
        sprintf(autoe_dirname, "mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images/%s",argv[2]);
        system(autoe_dirname);
        */    
        /*
        system(autoe_dirname);system("mkdir /home/polonator/G.007/G.007_acquisition/autoexp_FL_images");
        */
        for(j = 0;j < atoi(argv[5]);j++)
        {
    	    maestro_open(&m_sock);
    	    maestro_gotostagealign_position(m_sock, 0, j);

	        for(i = 0;i < 15;i++)
	        {
		        attempt_counter = 0;
		        autoexp_gain = atoi(argv[4])+i*10;
	    	    sprintf(autoe_filename, "   ... start acquring FL image in lane %d, for %s with autoexposure gain of %d ...   ",j, argv[2],autoexp_gain);
	    	    p_log_simple(autoe_filename);
	    	    sprintf(autoe_filename, "/home/polonator/G.007/G.007_acquisition/autoexp_FL_images/%s/%d_image_%d.raw",argv[2],j,autoexp_gain);
	    	    p_log_simple(autoe_filename);
	    	    wait_counter = 0;
	    	    snap(atof(argv[3]), autoexp_gain, argv[2], autoe_filename); 
	    	    sprintf(autoe_filename, "   ... acquired FL image in %d ms ...\n", wait_counter);
	    	    p_log_simple(autoe_filename);
	    
	    	    while((attempt_counter < 4) && (attempt_counter > 0))
	    	    {
		 	        sprintf(autoe_filename, "... ACQUIRING FAILED !!! Re-acquring FL image in lane %d, for %s with autoexposure gain of %d ...\n",atoi(argv[5]), argv[2],atoi(argv[4]));
	    	    	p_log_errorno(autoe_filename);
		    	    snap(atof(argv[3]), atof(argv[4]), argv[2], autoe_filename);     
	    	    }
	        } // end for i	
        } // end for j
    } // end else if

    /* Acquire 4 fluorescence images */
    else if((strcmp(argv[1], "colorsnap") == 0) && (argc == 7)){
        snap(atof(argv[2]), atof(argv[3]), "fam", "colorsnap-fam.raw");
        snap(atof(argv[2]), atof(argv[4]), "cy5", "colorsnap-cy5.raw");
        snap(atof(argv[2]), atof(argv[5]), "cy3", "colorsnap-cy3.raw");
        snap(atof(argv[2]), atof(argv[6]), "txred", "colorsnap-txr.raw");
    }

    else if(strcmp(argv[1], "complete-scan") == 0){
        maestro_open(&m_sock);
        maestro_stop(m_sock);
    }

    else if(strcmp(argv[1], "darkfield-off") == 0){
        maestro_open(&m_sock);
        maestro_darkfield_off(m_sock);
    }

    else if(strcmp(argv[1], "darkfield-on") == 0){
        maestro_open(&m_sock);
        maestro_darkfield_on(m_sock);
    }
      
    else if(strcmp(argv[1], "hometheta") == 0){
        maestro_open(&m_sock);
        maestro_hometheta(m_sock);
    }

    else if(strcmp(argv[1], "unlocktheta") == 0){
        maestro_open(&m_sock);
        maestro_unlocktheta(m_sock);
    }

    else if(strcmp(argv[1], "status") == 0){
        maestro_open(&m_sock);
        maestro_getstatus(m_sock);
    }

    else if(strcmp(argv[1], "power-on") == 0){
        network_iboot_on(&m_sock);
    }

    else if(strcmp(argv[1], "power-off") == 0){
        network_iboot_off(&m_sock);
    }

    else if(strcmp(argv[1], "gotostagealignpos") == 0){
        maestro_open(&m_sock);
        maestro_gotostagealign_position(m_sock, atoi(argv[2]), atoi(argv[3]));
    }

    else
    {
        main(1, argv);
    }
} // end function


void snap(float exposure, float gain, char *color, char *filename){
    int m_sock;
    short unsigned int *image;
    int imagemean;
    int shutterflag;
    FILE *outfile;

    wait_counter = 0;

    // open hardware and file 
    py_cameraInit(0); // use non-TDI config file 
    py_set_gain(gain);
    maestro_open(&m_sock);
    outfile = fopen(filename, "w");


    // configure hardware  
    maestro_setcolor(m_sock, color);
    //maestro_darkfield_off(m_sock);
    //py_set_gain(gain);moved to line 174

    // determine whether or not to use the shutter
    if(!strcmp(color, "none")){
        shutterflag = 0;
        maestro_darkfield_on(m_sock);
    }
    else{
        shutterflag = 1;
        maestro_darkfield_off(m_sock);
    }

    // setup the software to receive an image from the camera 
    py_setupSnap();


    // snap the image
    maestro_snap(m_sock, (int)(exposure * 1000.0), shutterflag);


    // wait for image to be received by framegrabber 
    while(!py_snapReceived()){
        wait_counter++;
        usleep(1000);
        if(wait_counter>20000){
        	attempt_counter++;
        	network_iboot_off(&m_sock);
        	sleep(1);
        	network_iboot_on(&m_sock);
        	sleep(1);
        	wait_counter = 0;
        }
    }

    // get pointer to image 
    image = py_getSnapImage();


    // calculate mean for informational purposes, then write image to file
    imagemean = py_imagemean(image);
    fprintf(stdout, "Image mean: %d\n", imagemean);
    fwrite(image, sizeof(short unsigned int), 1000000, outfile);
    fprintf(stdout, "finish outputing image");
    // close hardware and file 
    if(!strcmp(color, "none")) maestro_darkfield_off(m_sock);
    fclose(outfile);
    fprintf(stdout, "closing camera");
    py_cameraClose();
} // end function

