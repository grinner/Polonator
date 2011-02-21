
/* =============================================================================
// 
// Polonator G.007 Selective Illuminate Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// control_D4000.c: functionality for controlling optical release
// hardware (currently UV exposure with DMD array, but written for portability)
//
// Original version -- 07-22-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/



/* Make sure that /home/polonator/G.007 is in the include path! */
#ifndef ILLUMINATE_HARDWARE_D4000
#define ILLUMINATE_HARDWARE_D4000
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include <G.007_acquisition/src/Polonator_logger.h>/home/polonator/G.007/G.007_acquisition/src/
#include "logger.h"
#include "illuminate_common.h"
#include "image_generator.h"         /* illum_ig_*() */
#include "alignment_functions.h"     /* illum_af_*() */

/* the following chooses the correct illuminate hardware, rhw_*() */
#ifdef ILLUMINATE_HARDWARE_D4000
#include "hardware_D4000.h"
#endif /* ILLUMINATE_HARDWARE_D4000 */

#include "control_D4000.h"





/**************************
// private global variables
***************************/
int illuminate_initialized = 0;    /* has system been initiazlied? */
int illuminate_enabled_state;

typedef struct mask_store_struct {
    int used;                      /* has this slot been loaded? */
    int transformed;               /* has mask been transformed to illumination coords? */
    IlluminateMask mask_camera;    /* mask in camera coords */
    IlluminateMask mask_illuminate;/* mask in illumination coords */
} mask_store_type;
mask_store_type mask_store[ILLUMINATE_MASK_STORE_NUM];



/**************************
// private global variables
***************************/
HardwareData_type HardwareData;

/*******************
// private functions
********************/


/******************
// public functions
*******************/
#ifdef __cplusplus
extern "C"
{
#endif /* __cpluspluc */



/* initialize the module and hardware */
/* This Allocates the memory for the frame buffer as well */

int illuminate_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight)
{
    int idx;
    
    /* if(illuminate_initialized) return 0; */

    /* start logger service */
    start_logger(ILLUMINATE_FUNCTIONS_LOGFILE, 1);
    
    /* initialize hardware, quit on failure */
    if(rhw_init(&HardwareData) < 0)
    { 
        return -1;
    }
    
    /* initialize image generator */
    if(illum_ig_init(HardwareData) < 0)
    {
        return -2;
    }
    
    /* initialize alignment functions */
    if(illum_af_init(IlluminateWidth, IlluminateHeight, CameraWidth, CameraHeight) < 0)
    { 
    	return -3;
    }
    
    /* initialize illuminate mask store */
    for(idx = 0; idx < ILLUMINATE_MASK_STORE_NUM; idx++)
    {
        mask_store[idx].used = 0;
    }
    illuminate_initialized = 1;
    return 0;

}

/* turns off the system up for regular illumination */
int illuminate_disable(void)
{
    int retval, fillval;
    
    /* call hardware-specific command */
    rhw_light_off();
    retval = rhw_disable();
    if(retval < 0)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: unable to disable illuminate hardware");
        return retval;
    }
    
    /* mark that hardware is in the disabled state */
    illuminate_enabled_state = 0;

    p_log("STATUS:\tPolonator_IlluminateFunctions: illuminate hardware disabled");
    
    return 0;
}



/* put the machine into illuminate mode. Does not display an image; call illuminate_expose() for that */
int illuminate_enable(void)
{
    int retval;
    
    /* call hardware-specific command */
    retval = rhw_enable();
    if(retval < 0)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: unable to enable illuminate hardware");
        return retval;
    }
    
    /* display the image currently prepared */
    /*illuminate_load_image();*/
    
    /* mark that hardware is in the disabled state */
    illuminate_enabled_state = 1;
    
    p_log("STATUS:\tPolonator_IlluminateFunctions: illuminate hardware enabled");
    
    return 0;
}



/* render the current image and display it on the device */
int illuminate_expose(void)
{
    unsigned char *my_FrameBuf;
    
    /* render and obtain frame buffer location */
    if((my_FrameBuf = illum_ig_render()) == NULL)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: unable to render image");
        return -1;
    }
    printf("Loaded image for exposure\n");
    return rhw_load_image(my_FrameBuf);
}

/* image construction is (OR-)cummulative, so frame buffer must be cleared before new images */
int illuminate_clear_framebuffer(void)
{
    return illum_ig_clear_framebuffer();
}

/* generate a illuminate image corresponding to the given objects in illuminate-coordinates */
/* MASK MUST BE BIT-BIGENDIAN (high bit to left), BYTE-LITTLENDIAN (earlier byte to left)! 
/*  (ie., the row 01 02 03 corresponds to the mask 000000010000001000000011) */
/* returns 0 */
/* REDO: we should be putting a MASK onto a FRAME */
int illuminate_select(IlluminateCoords_type *cam_coords, int num_coords, int mask_no)
{
    IlluminateCoords_type *illum_coords;
    IlluminateMask_row *illum_mask;
    
    /* transform coords from camera-space to illuminate-space */
    illum_coords = malloc(num_coords * sizeof(IlluminateCoords_type));
    illum_af_transform_coords(illum_coords, cam_coords, num_coords);
    //illum_coords = cam_coords;

    /* transform mask, if needed */
    
    if((illum_mask = illuminate_mask_get(mask_no)) == NULL)
    {
        illum_mask = (mask_store[0].mask_illuminate);  // make sure that we have a valid pointer on error
    }
    
    /* generate the image */
    illum_ig_select(illum_coords, num_coords, illum_mask);
    
    /* illuminate allocated memory */
    free(illum_coords);

    return 0;
}


/* generate a circular illuminate mask by specifying radius.  Units are in pixel resolution.
The mask can be specified as either in camera or illuminate coordinates and stored in one of several
cache slots */
int illuminate_mask_radius(int rad, int which_coords, int mask_no)
{
    IlluminateMask mask;
    char message[200];
    if (mask_store[mask_no].used != 1)
    {
    	mask = illum_allocate_mask();
    	mask_store[mask_no].used = 1;   /* mark that slot is used */
    }	
    else
    {
        mask_store[mask_no].transformed = 0;   /* mask being overwritten so clear transform flag */
    }
    /* let the image generator generate the mask */
    if(illum_ig_mask_radius(mask, rad) < 0)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: illum_ig_mask_radius() failed");
        return -2;
    }

    sprintf(message, "STATUS:\tPolonator_IlluminateFunctions: generated mask with radius %d (in sub-bit resolution units)", rad);
    p_log(message);
    
    /* insert the mask into the mask store */
    return illuminate_mask_load(mask, which_coords, mask_no);
}



/* loads the given illuminate mask into memory as the given mask number.  The mask can be specified
as either in camera or illuminate coordinates. */
int illuminate_mask_load(IlluminateMask mask, int which_coords, int mask_no)
{
    char message[200];
    
    /* make sure that mask_store isn't about to be overrun */
    if((mask_no >= ILLUMINATE_MASK_STORE_NUM) || (mask_no < 0))
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: mask_no exceeds mask store size in illuminate_mask_load()");
        return -1;
    }


    if(which_coords == COORDS_CAMERA)
    {
	//memcpy(&mask_store[mask_no].mask_camera, mask, sizeof(IlluminateMask));
        mask_store[mask_no].mask_camera = mask;
        mask_store[mask_no].transformed = 0;    /* mask still needs transformation */
    }
    else if(which_coords == COORDS_RELEASE)
    {     
	//memcpy(&mask_store[mask_no].mask_illuminate, mask, sizeof(IlluminateMask));
        mask_store[mask_no].mask_illuminate = mask;
        mask_store[mask_no].transformed = 1;    /* mask is already in illuminate coords */
    }
    else
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: unknown coordinate-system in illuminate_mask_load()");
        return -2;
    }
    
    sprintf(message, "STATUS:\tPolonator_IlluminateFunctions: stored mask in slot #%d", mask_no);
    p_log(message);    
}


/* gets a pointer to a illumination mask in the mask store. Transforms the mask from camera coords if needed */
IlluminateMask_row *illuminate_mask_get(int mask_no)
{
    IlluminateMask mask;
    char message[200];
    
    /* make sure that mask_no can fit in mask_store */
    if((mask_no >= ILLUMINATE_MASK_STORE_NUM) || (mask_no < 0))
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: mask_no exceeds mask store size in illuminate_mask_get()");
        return NULL;
    }

    if(!mask_store[mask_no].used)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: requested mask has not been loaded");
        return NULL;
    }
    
    /* transform if needed */
    
    if(!mask_store[mask_no].transformed)
    {
        sprintf(message, "STATUS:\tPolonator_IlluminateFunctions: transforming mask in slot #%d", mask_no);
        p_log(message);
        mask = illum_allocate_mask();
        illum_af_transform_mask(mask, mask_store[mask_no].mask_camera);  // transform 
        illuminate_mask_load(mask, COORDS_RELEASE, mask_no);           //store in mask_store
    }
    
    return mask_store[mask_no].mask_illuminate;
}



/* Instruct the alignment module to load the given transformation parameters */
int illuminate_alignment_load(float *al_params, int param_num)
{
    p_log("STATUS:\tPolonator_IlluminateFunctions: loading camera-to-illuminate hardware alignment parameters");
    
    return illum_af_load(al_params, param_num);
}



/* load identify-map alignment/transformation parameters (useful for initial focus and alignment) */
int illuminate_alignment_load_identity(void)
{
    p_log("STATUS:\tPolonator_IlluminateFunctions: loading identity-map camera-to-illuminate hardware alignment parameters");
    
    return illum_af_load_identity();
}

/************** start python hooks ***************************************/
/*************************************************************************/

/* Clears all memory associated with illuminating an image or generating an image */
void py_clear_memory(void)
{
    py_clear_framebuffer();		/* Might as well clear this as well */
}

void py_clear_framebuffer(void)
{
    rhw_clear_mem();
    illuminate_clear_framebuffer();
}

/* sets the system up for full on regular illumination */
int py_light_all(void)
{
    int retval, fillval;

    /* call hardware-specific command */
    rhw_light_on();
    retval = rhw_disable();
    if(retval < 0)
    {
        p_log("ERROR:\tPolonator_IlluminateFunctions: unable to disable illuminate hardware");
        return retval;
    }

    /* mark that hardware is in the disabled state */
    illuminate_enabled_state = 0;

    p_log("STATUS:\tPolonator_IlluminateFunctions: illuminate hardware disabled");

    return 0;
}


void py_clear_mask(int mask_no)
{
    if (mask_store[mask_no].used == 1)
    {
        mask_store[mask_no].used = 1;
        mask_store[mask_no].transformed = 0;
	//illum_free_mask(mask_store[mask_no].mask_illuminate);
	//illum_free_mask(mask_store[mask_no].mask_camera);
    }
}

int py_illuminate_select(IlluminateCoords_type *cam_coords, int num_coords, int mask_no)
{
	return illuminate_select(cam_coords, num_coords, mask_no);
}

int py_illuminate_expose(void)
{
	return illuminate_expose();
}

int py_illuminate_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight)
{
	return illuminate_init(IlluminateWidth, IlluminateHeight, CameraWidth, CameraHeight);
}
int py_illuminate_enable(void)
{
	return illuminate_enable();
}

int py_illuminate_float(void)
{
    return rhw_float();
}

int py_illuminate_disable(void)
{
	return illuminate_disable();
}

/* Instruct the alignment module to load the given transformation parameters */
int py_illuminate_alignment_load(float *al_params, int param_num)
{
	return illuminate_alignment_load(al_params, param_num);
}

/* load identify-map alignment/transformation parameters (useful for initial focus and alignment) */
int py_illuminate_alignment_load_identity(void)
{
	return illuminate_alignment_load_identity();
}

/* Wrapper for finding a spot */
int py_illuminate_find_spot(unsigned short * image, IlluminateCoords_type *coords)
{
	return illum_af_find_spot(image, coords);	/* From AlignmentFunctions.c */
}

/*Test if this thing is working */
void py_illuminate_test_image(void)
{

	int IlluminateWidth  = 1920;
	int IlluminateHeight = 1080;
	int CameraWidth      = 1000;
	int CameraHeight     = 1000;
	int tester = 1;
	IlluminateCoords_type * squarez = calloc(10000,sizeof(IlluminateCoords_type ));
	int mask_no = 0;

	tester = py_illuminate_init(IlluminateWidth, IlluminateHeight, CameraWidth, CameraHeight);
	if (tester != 0)
	{
		fprintf(stdout, "Initialized failed \n");
	}
	fprintf(stdout, "Initialized the Illuminator\n");
	// load an identity map for alignment parameters
	illuminate_alignment_load_identity();
	py_illuminate_enable();
	fprintf(stdout, "Illuminator enabled\n");
	//py_clear_mask(mask_no);
	//py_clear_framebuffer();
	fprintf(stdout, "Frame buffer buffed\n");
	//drawsquare(squarez);
	fprintf(stdout, "Drew a square\n");
	// generate a circular Illuminate mask by specifying radius.  Units are in SUBBIT_RES resolution.
	// need to figure out where to create mask
	//IlluminateMask single_pixel_mask;
	//illum_ig_mask_radius(single_pixel_mask, mask_no);	// from ImageGenerator.c
	//illum_ig_select(squarez, 10000, single_pixel_mask);
	generate_image();

	fprintf(stdout, "put square in Frame buffer\n");
	py_illuminate_expose();
	fprintf(stdout, "Frame buffer exposed\n");
	py_clear_mask(mask_no);
	free(squarez);
	//py_illuminate_disable();
}

void drawsquare(IlluminateCoords_type * picture)
{
	int ind;
	int y_i;
	int x_i;
	for(y_i = 0; y_i < 100; y_i++)
	{
		for(x_i = 0; x_i < 100; x_i++)
		{
			ind = y_i+y_i*x_i;
			picture[ind].x = x_i;
			picture[ind].y = y_i;
		}
	}
}

void drawcircle(IlluminateCoords_type * picture)
{
	int ind = 0;
	int y_i;
	int x_i;
	float x_f;
	float r = 99;

	for (x_i = 0; x_f < (r+1); x_i++)
	{
		y_i = (int) (sqrt(r*r-x_f*x_f));
		x_f += 1;
		ind++;
		picture[ind].x = x_i;
		picture[ind].y = y_i;
	}
}

/* create the image to be displayed during focus */
void generate_image(void)
{
    /* circles 5 units in radius 10 units apart */

    int x, y;
    int x_step, y_step, my_rad;
    IlluminateCoords_type my_coord;

    x_step = 20;// * SUBBIT_RES;
    y_step = 20;// * SUBBIT_RES;
    my_rad = 0;// * SUBBIT_RES;


    /* generate a mask of the specified radius */
    illuminate_mask_radius(my_rad, COORDS_RELEASE, 0);

    /* clear the frame buffer before adding spots */
    illuminate_clear_framebuffer();

    /* place circles on a grid */
    for(y = 0; y < HardwareData.Height; y+=y_step)
    {
        for(x = 0; x < HardwareData.Width; x+=x_step)
        {
            my_coord.x = x;
            my_coord.y = y;
            illuminate_select(&my_coord, 1, 0);
        }
    }
}

int py_illum_mask_radius(int rad, int mask_no)
{
	illuminate_mask_radius(rad, COORDS_RELEASE, mask_no);
	return 1;
}

/* Illuminates just one pixel 			*/
/* Does not store the Mask in memory 	*/
void py_illuminate_pixel(IlluminateCoords_type *coords)
{
	/* generate a circular Illuminate mask by specifying radius.  Units are in SUBBIT_RES resolution. */
	// need to figure out where to create mask
    IlluminateMask single_pixel_mask;
    illum_ig_mask_radius(single_pixel_mask, 0);	/* from ImageGenerator.c */
    //illum_ig_select(coords, 1, single_pixel_mask);
    illuminate_select(coords, COORDS_RELEASE, 0);
}

/* add a point to be displayed during focus */
void py_illuminate_point(int x, int y, int mask_num)
{

    IlluminateCoords_type my_coord;

    my_coord.x = x;
    my_coord.y = y;
    illuminate_select(&my_coord, 1, mask_num);
}

/* generate a illuminate image corresponding to the given objects in illuminate-coordinates */
/* MASK MUST BE BIT-BIGENDIAN (high bit to left), BYTE-LITTLENDIAN (earlier byte to left)! 
/*  (ie., the row 01 02 03 corresponds to the mask 000000010000001000000011) */
/* returns 0 */
/* REDO: we should be putting a MASK onto a FRAME */
int py_illuminate_vector(int *vx, int *vy, int num_coords, int mask_no)
{
    IlluminateMask_row *illum_mask;

    /* transform mask, if needed */
    
    if((illum_mask = illuminate_mask_get(mask_no)) == NULL)
    {
        illum_mask = (mask_store[0].mask_illuminate);  // make sure that we have a valid pointer on error
    }
    
    /* generate the image */
    return illum_ig_points(vx,vy, num_coords, illum_mask);
   
}

int py_illuminate_spot_find(unsigned short *image, int *x, int *y)
{
    int val;
    IlluminateCoords_type my_coord; 
    val = illum_af_find_spot(image, &my_coord);	/* From AlignmentFunctions.c */
    *x = my_coord.x;
    *y = my_coord.y;
    return val;
}

#ifdef __cplusplus
}
#endif /* __cpluspluc */

