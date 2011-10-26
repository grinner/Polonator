/* =============================================================================
//
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Polonator-stagealign.c: does an expensive image alignment operation to determine
// a global offset for all stage coordinates to keep the images in close to perfect
// register with the object table so that realtime alignment can be fast
//
// Release 1.0 -- 05-06-2008
// Release 1.1 -- 06-19-2008 modified to handle two flowcells [GP]
// Release 2.0 -- 12-01-2008 modified for PolonatorScan controller code [GP]
// Release 2.1 -- 01-07-2009 modified to perform alignment on all lanes [GP]
// Release 2.11-- 01-29-2009 modified to fix offset sign bug [GP]
// Release 2.2 -- 02-26-2009 modified for new image snap procedure [GP]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "common.h"
#include "as_phoenix_functions.h"
#include "maestro_functions.h"
#include "logger.h"
#include "config.h"
#include "Polonator-stagealign.h"

/* GLOBAL DECLARATIONS */
/**/
char log_string[500];
char output_directory[500];
char autoe_filename[500];
char autoe_dirname[500];
char log_dir[255];

int main(int argc, char *argv[])
{
    int initialize_flag;
    char config_value[255];
    int i;
    int total_lanes;

    char acqcfgpath[127];

    char command_buffer[255];
    char base_dir[255];
    sprintf(base_dir, "%s/polonator/G.007/acquisition", getenv("HOME"));
    sprintf(log_dir, "%s/logs", base_dir);
    sprintf(command_buffer, "mkdir -p %s", log_dir);
    system(command_buffer);

    if(argc == 3){
        initialize_flag = atoi(argv[2]);
    }
    else{
        initialize_flag = 0;
    }
    fprintf(stdout, "the initialize flag is %d\n", initialize_flag);

    /* Open config file */
    strcpy(acqcfgpath, getenv("POLONATOR_PATH"));
    strcat(acqcfgpath, "/config_files/polonator-acq.cfg");
    config_open(acqcfgpath);
    if(!config_getvalue("stagealign_wells_per_fc", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_wells_per_fc) returned no value");
        exit(0);
    }
    total_lanes = atoi(config_value);
    if(!config_getvalue("stagealign_outputdir", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_outputdir) returned no value");
        exit(0);
    }
    strcpy(output_directory, base_dir);
    strcat(output_directory, "/");
    strcat(output_directory, config_value);
    config_close();

    fprintf(stdout, "=%s=\n", output_directory);

    /* Make sure stagealign output directory exists */
    mkdir(output_directory, S_IRWXU);
    sprintf(log_string, "Create new directory %s :", output_directory);
    p_log_simple(log_string);


    for(i = 0; i < total_lanes; i++)
    {
        stagealign(atoi(argv[1]), i, initialize_flag);
    }
    return 0;
}


void stagealign(int fcnum, int lane_num, int initialize)
{

    short unsigned int *testimage;
    short unsigned int *baseimage;
    FILE *baseimgfp;
    FILE *offsetfp;
    /* used to dump score matrix to file when debugging */
    FILE *score_matrixfp;

    /* Maestro declarations */
    int m_sock; /* socket for the Maestro controller */

    char config_value[255];
    char logfilename[255];
    char offsetfilename[255];
    char stagealign_baseimgfilename[255];

    char acqcfgpath[255];

    /* Used to send commands and receive responses from Maestro */
    char command[255];
    char response[255];
    int response_length;

    /* Image acquisition settings, populated from config file */
    int stagealign_integration_inmsec;
    int stagealign_gain;
    int stagealign_well;
    int stagealign_wells_per_fc;

    /* Values used for conversion between pixels and stage units */
    int stagealign_optical_mag;
    int ccd_pixel_size;
    double pixelsize_at_stage; /* size of a pixel at the stage in microns */

    /* Hold offsets found by alignment in pixels and stageunits */
    int pixel_offset_x, pixel_offset_y;
    double stageunit_offset_x, stageunit_offset_y;
    int lane_index;

    /* Holds previous offsets from controller in case the alignment doesn't work */
    int curr_offset_x, curr_offset_y;

    /* Holds encoder resolutions from controller, used for calculating distance of move */
    double encoder_res_X, encoder_res_Y;


    /* 'score' of best alignment */
    int score;

    /* Largest pixel offset allowable _after_ the adjustment has been made */
    /* if there is still a shift larger than this, conclude something is wrong */
    /* and go back to the previous offset */
    int successful_move_threshold = 150; /*RCT was = 12*/

    int i;

  /* in debugging mode, dump internal data to files */
#ifdef DEBUG_STAGEALIGN
    FILE *imgfp;
    sprintf(command, "%s/stagealign-image%d_%d.raw", output_directory, fcnum, lane_num);
    imgfp = fopen(command, "w");
    sprintf(command, "%s/stagealign-scorematrix%d_%d", output_directory, fcnum, lane_num);
    score_matrixfp = fopen(command, "w");
#endif
    // p_log_simple("awesome2\n");
    /* Open config file */
    strcpy(acqcfgpath, getenv("POLONATOR_PATH"));
    strcat(acqcfgpath, "/config_files/polonator-acq.cfg");
    config_open(acqcfgpath);
    // p_log_simple("awesome1\n");
    /* Initialize variables */
    if(!config_getvalue("stagealign_logfilename", config_value)){
        fprintf(stderr, "ERROR:\tPolonator-stagealign: config_getval(key logfilename) returned no value\n");
        exit(0);
    }
    // p_log_simple("awesome0\n");
    strcpy(logfilename, log_dir);
    strcat(logfilename, "/");
    strcat(logfilename, config_value);
    sprintf(command, "%d", fcnum);
    strcat(logfilename, command);
    strcat(logfilename, ".log");
    // p_log_simple(logfilename);
    start_logger(logfilename, 1);

    strcpy(offsetfilename, log_dir);
    strcat(offsetfilename, "/");
    strcat(offsetfilename, config_value);
    strcat(offsetfilename, command);
    strcat(offsetfilename, ".offsetlog");
    fprintf(stdout, offsetfilename);
    fflush(stdout);
    /*
    if this is being run in 'initialize mode' -- the first scan of a run --
    overwrite the offset logfile
    */
    // p_log_simple("awesome66\n");
    if(initialize){
        offsetfp = fopen(offsetfilename, "w");
    }
    else{
        offsetfp = fopen(offsetfilename, "a");
    }
    // p_log_simple("awesome00\n");
    if(!config_getvalue("stagealign_baseimgfilename", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_baseimgfilename) returned no value");
        exit(0);
    }
    sprintf(stagealign_baseimgfilename, "%s/%s%s_%d.raw", output_directory, config_value, command, lane_num);
    p_log_simple("awesome01\n");
    if(!config_getvalue("stagealign_integration_inmsec", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_integration_inmsec) returned no value");
        exit(0);
    }
    stagealign_integration_inmsec = atoi(config_value);
    // p_log_simple("awesome02\n");
    if(!config_getvalue("stagealign_gain", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_gain) returned no value");
        exit(0);
    }
    stagealign_gain = atoi(config_value);
    // p_log_simple("awesome03\n");
    if(!config_getvalue("stagealign_optical_mag", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_optical_mag) returned no value");
        exit(0);
    }
    stagealign_optical_mag = atoi(config_value);
    // p_log_simple("awesome04\n");
    if(!config_getvalue("stagealign_ccd_pixel_size", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_ccd_pixel_size) returned no value");
        exit(0);
    }
    ccd_pixel_size = atoi(config_value);
    // p_log_simple("awesome05\n");
    if(!config_getvalue("stagealign_wells_per_fc", config_value)){
        p_log("ERROR:\tPolonator-stagealign: config_getval(key stagealign_wells_per_fc) returned no value");
        exit(0);
    }
    stagealign_wells_per_fc = atoi(config_value);
    config_close();
    // p_log_simple("awesome06\n");
    stagealign_well = lane_num;
    lane_index = (fcnum * stagealign_wells_per_fc) + stagealign_well;


    baseimage = (short unsigned int*)malloc(1000000 * sizeof(short unsigned int));

    /*--------------------------------------------------------------------------
    //
    // MAESTRO SETUP
    /*/
    p_log_simple("STATUS:\tPolonator-stagealign: Opening connection to Maestro...");
    maestro_open(&m_sock);
    /*
    //--------------------------------------------------------------------------
    */


    /*--------------------------------------------------------------------------
    //
    // CAMERA SETUP
    /*/
    p_log_simple("STATUS:\tPolonator-stagealign: Opening camera handle...");
    camera_init();
    set_gain(stagealign_gain);
    setup_snap(); /* setup capture software to wait for images from camera */
    /*
    //--------------------------------------------------------------------------
    */


    /*p_log("STATUS:\tPolonator-stagealign: Darkfield illuminator on...");  rolony*/
    /*maestro_darkfield_on(m_sock);
    p_log("STATUS:\tPolonator-stagealign: Select darkfield filter block...");  rolony*/
    maestro_setcolor(m_sock, "cy5");


    /* IF INITIALIZING, RESET OFFSETS */
    if(initialize){
        p_log_simple("INITIALIZING STAGEALIGN");
        sprintf(command, "PolonatorScan.OFFSET_X[%d]=0\n\r", lane_index);
        p_log_simple(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log_simple(response);

        sprintf(command, "PolonatorScan.OFFSET_Y[%d]=0\n\r", lane_index);
        p_log_simple(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log_simple(response);
    }
    /* GET OFFSETS IN CASE ALIGNMENT FAILS */
    else{
        p_log_simple("Storing current offsets...");
        sprintf(command, "PolonatorScan.OFFSET_X[%d]\n\r", lane_index);
        p_log_simple(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log_simple(response);
        curr_offset_x = atoi(response);

        sprintf(command, "PolonatorScan.OFFSET_Y[%d]\n\r", lane_index);
        p_log_simple(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log_simple(response);
        curr_offset_y = atoi(response);
    }

    /* MOVE STAGE TO ORIGIN */
    maestro_gotostagealign_position(m_sock, fcnum, lane_num);

    p_log_simple("awesome fool00\n");
    /* ACQUIRE IMAGE */
    p_log("STATUS:\tPolonator-stagealign: Acquire image...");
    maestro_snap(m_sock, stagealign_integration_inmsec, 1); /*rolony*/
    while(!py_snapReceived()){;}
    testimage = get_snap_image();


    /* IF INITIALIZING, RE-WRITE THE BASE IMAGE; THE OFFSET FOUND SHOULD BE ZERO */
    p_log_simple(stagealign_baseimgfilename);
    if(initialize){
        baseimgfp = fopen(stagealign_baseimgfilename, "w");
        fwrite(testimage, 1000000, sizeof(short unsigned int), baseimgfp);
        fclose(baseimgfp);
    }
    p_log_simple("awesome fool01\n");
#ifdef DEBUG_STAGEALIGN
    fwrite(testimage, 1000000, sizeof(short unsigned int), imgfp);
#endif

    /* LOAD BASE IMAGE */
    p_log("STATUS:\tPolonator-stagealign: Load base image and determine offset...");
    baseimgfp = fopen(stagealign_baseimgfilename, "r");
    fread(baseimage, 1000000, sizeof(short unsigned int), baseimgfp);
    fclose(baseimgfp);
    p_log("STATUS:\tPolonator-stagealign: Load base image and determine offset2...");

    /* DETERMINE OFFSETS */
    register_image(baseimage, testimage, &pixel_offset_x, &pixel_offset_y, &score, score_matrixfp);
    sprintf(log_string, "STATUS:\tPolonator-stagealign: Found pixel offsets X:%d, Y:%d, score:%d", pixel_offset_x, pixel_offset_y, score);
    p_log(log_string);


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
    pixelsize_at_stage = ((double)ccd_pixel_size / (double)stagealign_optical_mag) / 1000;
    stageunit_offset_x = (double)pixel_offset_x * pixelsize_at_stage * encoder_res_X * -1;
    stageunit_offset_y = (double)pixel_offset_y * pixelsize_at_stage * encoder_res_Y * -1;


    /* SET NEW OFFSETS ON CONTROLLER USING VALUES */
    /* CALCULATED ABOVE */
    sprintf(command, "PolonatorScan.OFFSET_X[%d]\n\r", lane_index);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    fprintf(offsetfp, "%d\t%d\t", fcnum, stagealign_well);
    fprintf(offsetfp, "%d\t", atoi(response));
    sprintf(command, "PolonatorScan.OFFSET_Y[%d]\n\r", lane_index);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    fprintf(offsetfp, "%d\t", atoi(response));
    fprintf(offsetfp, "%d\t%d\t%d\t%d\t", (int)stageunit_offset_x, (int)stageunit_offset_y, pixel_offset_x, pixel_offset_y);


    /* ISSUE COMMANDS TO ADJUST STAGE COORDS */
    p_log("STATUS:\tPolonator-stagealign: Set offset variables on Maestro...");
    sprintf(command, "PolonatorScan.OFFSET_X[%d]=PolonatorScan.OFFSET_X[%d]+%d\n\r", lane_index, lane_index, (int)stageunit_offset_x);
    p_log(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);

    sprintf(command, "PolonatorScan.OFFSET_Y[%d]=PolonatorScan.OFFSET_Y[%d]+%d\n\r", lane_index, lane_index, (int)stageunit_offset_y);
    p_log(command);
    send(m_sock, command, strlen(command), 0);
    maestro_readresponse(m_sock, response, &response_length);
    p_log(response);


    /* MOVE, THEN ACQUIRE ANOTHER IMAGE TO VERIFY OFFSET WORKED */
    /* maestro_gotostagealign_position(m_sock, fcnum, lane_num);
    maestro_snap(m_sock, stagealign_integration_inmsec, 0);
    while(!py_snapReceived()){;}
    testimage = get_snap_image();
    */

#ifdef DEBUG_STAGEALIGN
    fwrite(testimage, 1000000, sizeof(short unsigned int), imgfp);
    fclose(imgfp);
#endif


    /* DETERMINE OFFSET TO CONFIRM */
    /* p_log("STATUS:\tPolonator-stagealign: Re-compute alignment to verify move...");
    register_image(baseimage, testimage, &pixel_offset_x, &pixel_offset_y, &score, score_matrixfp);
    sprintf(log_string, "STATUS:\tPolonator-stagealign: Found pixel offsets X:%d, Y:%d, score:%d", pixel_offset_x, pixel_offset_y, score);
    p_log(log_string);
    fprintf(offsetfp, "%d\t%d", pixel_offset_x, pixel_offset_y);
    */

    /* DID THE MOVE WORK? */
    if(((abs(pixel_offset_x)>successful_move_threshold) || (abs(pixel_offset_y)>successful_move_threshold)) && (!initialize))
    {
        sprintf(log_string, "ERROR:\tPolonator-stagealign: one or more offsets are greater that the %d-pixel maximum; X:%d, Y:%d",
            successful_move_threshold,
            pixel_offset_x,
            pixel_offset_y);
        p_log(log_string);
        fprintf(offsetfp, "*");
        /* mark current line in offsetlog, since offsets found will not be the offsets stored on the controller */

        sprintf(log_string, "Restoring previous offsets X:%d, Y:%d", curr_offset_x, curr_offset_y);


        sprintf(command, "PolonatorScan.OFFSET_X[%d]=%d\n\r", lane_index, curr_offset_x);
        p_log(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log(response);

        sprintf(command, "PolonatorScan.OFFSET_Y[%d]=%d\n\r", lane_index, curr_offset_y);
        p_log(command);
        send(m_sock, command, strlen(command), 0);
        maestro_readresponse(m_sock, response, &response_length);
        p_log(response);
    }

    /* EXIT */
#ifdef DEBUG_STAGEALIGN
    fclose(score_matrixfp);
#endif
    fprintf(offsetfp, "\n");
    fclose(offsetfp);
    /*maestro_darkfield_off(m_sock); rolony*/
    camera_close();
    free(baseimage);
    close_logger();
    p_log_simple("awesome fool02\n");
}


/*
returns offset, in pixels, between base_img and test_img for translation
   to bring images into register
*/
void register_image(short unsigned int *base_img,
		    short unsigned int *test_img,
		    int *x_offset,
		    int *y_offset,
		    int *score,
		    FILE *score_matrix_fp)
{

    int window_width = 100;
    int i, j, k, l;

    int a;
    int *score_matrix;
    int *flat_score_matrix;
    int score_matrix_index;

    int curr_score, best_score;

    int test_x_idx, test_y_idx, base_x_idx, base_y_idx;

    score_matrix = (int*)malloc((1000-window_width)*(1000-window_width)*sizeof(int));
    flat_score_matrix = (int*)malloc((1000-window_width)*(1000-window_width)*sizeof(int));
    score_matrix_index = 0;

    *x_offset = -10000;
    *y_offset = -10000;
    a = 500 - (window_width / 2);
    for(i = 0; i < 1000-window_width; i+=1)
    { /* y rows */
        for(j = 0; j < 1000-window_width; j+=1)
        { /* x cols */

            curr_score = 0;

            for(k=0; k<window_width; k+=2)
            { /* y rows */
	            for(l=0; l<window_width; l+=2)
	            { /* x cols */

                    test_y_idx = k + a;
                    test_x_idx = l + a;
                    base_y_idx = k + i;
                    base_x_idx = l + j;

                    /* compute current difference */
                    curr_score += (abs(*(test_img + ((1000 * test_x_idx)+test_y_idx)) \
                                - *(base_img + ((1000 * base_x_idx)+base_y_idx))));
	            } // end for l
            } // end for k

            *(score_matrix + score_matrix_index) = curr_score;
            score_matrix_index++;
        } // end for j
    } // end for i

    /* now, flatten score matrix, then determine location of minimum */
    /* sum of differences will be minimized when the images are in register */
    flatten_image(score_matrix, flat_score_matrix, 1, 1000-window_width);

    best_score = INT_MAX;
    score_matrix_index = 0;
    for(i = 0; i < 1000-window_width; i++)
    {
        for(j = 0; j < 1000-window_width; j++)
        {
            if(*(flat_score_matrix + score_matrix_index) < best_score)
            {
	            best_score = *(flat_score_matrix + score_matrix_index);
	            *x_offset = j - a;
	            *y_offset = i - a;
	            fprintf(stdout, "%d %d %d\n", best_score, *x_offset, *y_offset);
            }
            score_matrix_index++;
        }
    }

#ifdef DEBUG_STAGEALIGN
    fwrite(score_matrix, (1000-window_width) * (1000-window_width), sizeof(int), score_matrix_fp);
    fwrite(flat_score_matrix, (1000-window_width) * (1000-window_width), sizeof(int), score_matrix_fp);
#endif

    if((*x_offset == -10000) || (*y_offset == -10000))
    {
        p_log("ERROR: register_image did not find good offset");
        *x_offset=0;
        *y_offset=0;
        *score = -1;
    }
    else
    {
        *score = best_score;
    }
    free(score_matrix);
    free(flat_score_matrix);
} // end function


void flatten_image(int* orig_image,
		   int* new_image,
		   int rescale,
		   int img_size){

    long int pixel_mean;
    int i, j, k, m;
    int *index, *index2, *index3;
    int *bg_image;
    int bg_min, bg_max;
    int new_min, new_max;
    int curr_sub;
    int new_val;

    int NUM_YROWS, NUM_XCOLS;

    NUM_YROWS = img_size;
    NUM_XCOLS = img_size;

    if( (bg_image = (int *)malloc(NUM_XCOLS*NUM_YROWS*(sizeof(int)))) == NULL){
        exit(0);
    }

    /* INITIALIZE NEW_IMAGE AND BG_IMAGE PIXELS SINCE NOT ALL WILL BE
    // ASSIGNED ELSEWHERE*/
    for(i = 0; i < NUM_YROWS*NUM_XCOLS; i++){
        *(new_image + i) = INT_MAX;
        *(bg_image + i) = 0;
    }


    /* FIRST, GENERATE 'BACKGROUND' IMAGE BY AVERAGING OVER A PIXEL WINDOW
    // OF SUFFICIENT SIZE TO OBSCURE BEADS (e.g. FLATTEN_WINDOWSIZE=5)
    // KEEP TRACK OF MEAN AND MAX PIXEL VALS HERE AS WELL*/
    bg_min = INT_MAX;
    bg_max = 0;
    for(i = FLATTEN_WINDOWSIZE; i < (NUM_YROWS-FLATTEN_WINDOWSIZE+1); i+=(FLATTEN_WINDOWSIZE*2))
    {
        for(j=FLATTEN_WINDOWSIZE; j<(NUM_XCOLS-FLATTEN_WINDOWSIZE+1); j+=(FLATTEN_WINDOWSIZE*2))
        {
            pixel_mean = 0;
            for(k=(-1*FLATTEN_WINDOWSIZE); k<(FLATTEN_WINDOWSIZE); k++)
            {
	            for(m=(-1*FLATTEN_WINDOWSIZE); m<(FLATTEN_WINDOWSIZE); m++)
	            {
	                pixel_mean+= *(orig_image + ((i + k)*NUM_XCOLS) + (j + m));
	            } // end for m
            } // end for k
            pixel_mean = (long int) (pixel_mean / (long int)(pow((FLATTEN_WINDOWSIZE*2),2)));
            for(k=(-1*FLATTEN_WINDOWSIZE); k<(FLATTEN_WINDOWSIZE); k++)
            {
	            for(m=(-1*FLATTEN_WINDOWSIZE); m<(FLATTEN_WINDOWSIZE); m++)
                {
	                *(bg_image + ((i + k)*NUM_XCOLS) + (j + m)) = pixel_mean;
	                if(pixel_mean < bg_min){
	                    bg_min = (int)pixel_mean;
	                }
	                if(pixel_mean > bg_max)
	                {
	                    bg_max = (int)pixel_mean;
	                }
	            } // end for m
            } // end for k
        } // end for j
    } // end for i

    /* RESCALE BG_IMAGE SO IT STARTS AT 0, SUBTRACT FROM IMAGE; COMPUTE
    // NEW IMAGE PIXEL MAX AND MIN IF RESCALE == 1*/
    new_min = INT_MAX;
    new_max = 0;
    for(i = 0; i<NUM_YROWS * NUM_XCOLS; i++)
    {
        index = orig_image + i;
        index2 = bg_image + i; /* background_image pointer*/
        index3 = new_image + i; /* new_image pointer*/

        new_val = *(index2) - bg_min;
        if(new_val < 0) new_val = 0;
        *(index2) = (int)new_val; /* this is the backgound pixel value*/


        /*    new_val = *(index) - *(index2);*/
        new_val = *(index) +(bg_max - *(index2));

        if(new_val < 0) new_val = 0;
        *(index3) = (int)new_val;
        if(rescale){
            if(*(index3) > new_max){
                new_max = *(index3);
            }
            if(*(index3) < new_min){
                new_min = *(index3);
            }
        }
    } // end for i
    if(new_min==INT_MAX) new_min=0;
    new_max = new_max - new_min;

    /* RESCALE IF NECESSARY*/
    if(rescale)
    {
        for(i = 0; i < (NUM_YROWS*NUM_XCOLS); i++)
        {
            index = new_image + i;
            *(index) = *(index) - new_min;
            *(index) = (int)((double)( *(index)) * (double)(INT_MAX/(double)(new_max)));
        }
    }
    free(bg_image);
} // end function

