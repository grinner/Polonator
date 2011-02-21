#ifndef _IMAGE_GEN_
#define	_IMAGE_GEN_

/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// ImageGenerator.h: generates images/exposure patterns for photorelease of
// selected objects
//
// Original version -- 07-22-2009 [DL]
// Modified by Nick Conway 11/23/2010 removed subbit stuff
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/



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
int illum_ig_init(HardwareData_type HardwareDescriptor);
int illum_ig_fill(int fillval);
unsigned char *illum_ig_render(void);
int illum_ig_clear_framebuffer(void);
int illum_ig_select(IlluminateCoords_type *coords, int num_coords, IlluminateMask mask);
int illum_ig_mask_radius(IlluminateMask mask, int rad);
void illum_ig_dump(unsigned char *image, int Width, int Height);
void SketchPadWide_to_FrameBuf(void);

int illum_ig_points(int *vx, int *vy, int num_coords, IlluminateMask mask);

#ifdef __cplusplus
}
#endif /* __cpluspluc */

#endif
