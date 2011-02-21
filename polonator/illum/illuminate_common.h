
/* =============================================================================
// 
// Polonator G.007 Selective Illuminate Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// Illuminate_common.h: common header file for several of the Polonator
// photorelease-related modules
//
// Original version -- 07-22-2009 [DL]
// Modifified by Nick Conway 11-23-2010
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


#ifndef ILLUMINATE_COMMON_H
#define ILLUMINATE_COMMON_H

#define MASK_DIM 4      /* each exposure spot corresponds to a blot at most ~2*MASK_DIM bytes in each dimension, in subbit units(!) */

/*!TODO  Add way to make this type size loaded from a config file runtime instead of a header file compile-time*/
/* for now it is 16 bit !!!! */
#define CAMERA_DATA_TYPE unsigned short

/* describe which coordinate system given coordinates are in */
#define COORDS_CAMERA   0
#define COORDS_RELEASE  1


typedef struct HardwareData_struct
{
	int Width;
	int Height;
	int BitsPerPixel;
} HardwareData_type;


typedef struct IlluminateCoords_struct /* these coordinates are in SUBBIT_RES resolution */
{
    signed short int x;
    signed short int y;
} IlluminateCoords_type;


#define ILLUMINATEMASK_ROW_LEN (2*MASK_DIM)
#define ILLUMINATEMASK_COL_LEN (2*MASK_DIM*8) // times 8 because we are storing bits in bytes for the mask data structure
//typedef unsigned char IlluminateMask_row[ILLUMINATEMASK_ROW_LEN];
//typedef IlluminateMask_row IlluminateMask[ILLUMINATEMASK_COL_LEN];   /* resulting array: 1st arg. is y, 2nd arg. is x */

typedef unsigned char * IlluminateMask_row;
typedef IlluminateMask_row * IlluminateMask;   /* resulting array: 1st arg. is y, 2nd arg. is x */

IlluminateMask illum_allocate_mask(void);
int illum_free_mask(IlluminateMask mask);

/* mask is specified such that if MASK_DIM=1 and SUBBIT_RES=0.25,
a mask selecting only the center spot would look like:

 [0000
  0000
  0010
  0000]
  
MASK MUST BE BIT-BIGENDIAN (high bit to left), BYTE-LITTLENDIAN (earlier byte to left)! 
(ie., the row 01 02 03 corresponds to the mask 000000010000001000000011)
*/








#endif /* ILLUMINATE_COMMON_H */

