#ifndef _CON_D4000_
#define _CON_D4000_

/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// control_D4000.h: functionality for controlling optical release
// hardware (currently UV exposure with DMD array, but written for portability)
//
// Original version -- 07-22-2009 [DL]
// Version 1.1		-- 01-27-2010 [NC] Wyss Institute Added Python hooks
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/



/* set default illuminate hardware */
#ifndef ILLUMINATE_HARDWARE_D4000
#define ILLUMINATE_HARDWARE_D4000
#endif /* ILLUMINATE_HARDWARE_D4000 */

#define ILLUMINATE_MASK_STORE_NUM  10  /* how many release masks can be loaded at once? */
#define ILLUMINATE_FUNCTIONS_LOGFILE "./Polonator_IlluminateFunctions.log"

#ifndef CAMERA_DIMENSIONS_GIVEN
#define CAMERA_IMAGE_WIDTH  1000
#define CAMERA_IMAGE_HEIGHT 1000
#endif


#ifdef __cplusplus
extern "C"
{
#endif /* __cpluspluc */


/******************
// global variables
*******************/



/******************
// global functions
*******************/
int illuminate_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight);
int illuminate_disable(void);
int illuminate_enable(void);
int illuminate_expose(void);
int illuminate_clear_framebuffer(void);
int illuminate_select(IlluminateCoords_type *coords, int num_coords, int mask_no);
int illuminate_mask_radius(int rad, int which_coords, int mask_no);
int illuminate_mask_load(IlluminateMask mask, int which_coords, int mask_no);
IlluminateMask_row *illuminate_mask_get(int mask_no);
int illuminate_alignment_load(float *al_params, int param_num);
int illuminate_alignment_load_identity(void);

/* Get SWIG-Y with it */
int py_illum_mask_radius(int rad, int mask_no);
void py_illuminate_pixel(IlluminateCoords_type *coords);
void py_clear_framebuffer(void); // depricate
void py_clear_memory(void);
int py_illuminate_expose(void); // depricate
void py_clear_mask(int mask_no);
int py_illuminate_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight);
int py_illuminate_select(IlluminateCoords_type *cam_coords, int num_coords, int mask_no);
int py_illuminate_enable(void); // depricate
int py_illuminate_disable(void); //depricate
int py_light_all(void);
int py_illuminate_float(void);


/* Derived from AlignmentFunctions.c */

int py_illuminate_alignment_load(float *al_params, int param_num);
int py_illuminate_alignment_load_identity(void);
int py_illuminate_find_spot(unsigned short *image, IlluminateCoords_type *coords);

void py_illuminate_test_image(void);
void drawcircle(IlluminateCoords_type * picture);
void drawsquare(IlluminateCoords_type * picture);
void generate_image(void);
void py_illuminate_point(int x, int y, int mask_num);

int py_illuminate_vector(int *vx, int *vy, int num_coords, int mask_no);
int py_illuminate_spot_find(unsigned short *image, int *x, int *y);

#ifdef __cplusplus
}
#endif /* __cpluspluc */

#endif
