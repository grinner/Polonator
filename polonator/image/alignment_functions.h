#ifndef _ALIGNFUNC
#define _ALIGNFUNC

/* =============================================================================
// 
// Polonator G.007 Selective Illuminate Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// alignment_functions.h: handles coordinate-system transformations between camera
// and release hardware, as well as helps determine this transformation
//
// Original version -- 07-29-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


/* alignment parameters are enumerated as follows:

[Rel_x]     [ A0 ]     [A2  A3]   [Cam_x - A6]     [ A8 ]
[     ]  =  [    ]  +  [      ] * [          ]  +  [    ]
[Rel_y]     [ A1 ]     [A4  A5]   [Cam_y - A7]     [ A9 ]

This parameters A7 to A10 have predetermined values corresponding to:

(A6, A7) = Center of camera image in x and y, respectively
(A8, A9) = Center of release image in x and y, respectively

The additive parameters (A0, A1, A6, A7, A8 & A9) are given in subbit resolution (defined in Illuminate_common.h)
The multiplicative parameters (A2, A3, A4 & A5) are in 1 / (1 << ALIGNMENT_PARAM_PADBITS) units (see below)
*/


/* fixed-point resolution for alignment parameters in terms of extra bits */
/* should be at least log2(X/2) where X is the larger of (camera width, camera height) */
#define ALIGNMENT_PARAM_PADBITS 10     /* smallest increment = 1/1048 */

#define ALIGNMENT_PARAM_NUM 10


#ifdef __cplusplus
extern "C"
{
#endif /* __cpluspluc */


/******************
// global variables
*******************/

#include "illuminate_common.h"

#define MASK_BITS (ILLUMINATEMASK_ROW_LEN*8*ILLUMINATEMASK_COL_LEN)

/******************
// global functions
*******************/
int illum_af_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight);
int illum_af_load(float *al_params, int param_num);
int illum_af_load_identity(void);
int illum_af_transform_coords(IlluminateCoords_type *rel_coords, IlluminateCoords_type *cam_coords, int num_coords);
int illum_af_transform_mask(IlluminateMask outmask, IlluminateMask inmask);
int illum_af_find_spot(unsigned short *image, IlluminateCoords_type *coords);





#ifdef __cplusplus
}
#endif /* __cpluspluc */

#endif
