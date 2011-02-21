
/*	=============================================================================
 *	Polonator G.007 Selective Release Software
 *
 *   Church Lab, Harvard Medical School
 *   Written by Daniel Levner
 *
 *   alignment_functions.c: handles coordinate-system transformations between camera
 *   and release hardware, as well as helps determine this transformation
 *
 *   Original version --                07-29-2009 [DL]
 *   modified variable/function names   02-02-2010 [NC]
 *
 *   This software may be modified and re-distributed, but this header must appear
 *   at the top of the file.
 *
 *	=============================================================================
*/


/* Make sure that /home/polonator/G.007 is in the include path! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <G.007_acquisition/src/Polonator_logger.h>*/
#include "logger.h"
#include "illuminate_common.h"
#include "alignment_functions.h"

/**************************/
/* private global variables*/
/***************************/

int IlluminateWidth;
int IlluminateHeight, CameraWidth, CameraHeight;
float al_data[ALIGNMENT_PARAM_NUM];



/*************************
// public global variables
**************************/




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
int illum_af_init(int my_IlluminateWidth, int my_IlluminateHeight, int my_CameraWidth, int my_CameraHeight)
{
    p_log("STATUS:\tAlignmentFunctions: initializing");
    
    /* store the image descriptors */
    IlluminateWidth  = my_IlluminateWidth;
    IlluminateHeight = my_IlluminateHeight;
    CameraWidth   = my_CameraWidth;
    CameraHeight  = my_CameraHeight;
    
    return 0;
}



/* loads alignment/transformation parameters */
int illum_af_load(float *al_params, int param_num)
{
    int idx;
    
    /* make sure that we have the correct number of parameters */
    if(param_num != ALIGNMENT_PARAM_NUM)
    {
        p_log("ERROR:\tAlignmentFunctions: incorrect number of parameters given to illum_load(). Parameter sets may be incompatible!");
        return -1;
    }
    
    /* load the data blindly */
    for(idx = 0; idx < ALIGNMENT_PARAM_NUM; idx++)
    {
        al_data[idx] = al_params[idx];
    } 
    return 0;
}



/* loads an identity map as the alignment/transformation parameters.
Useful for finding focus and initial alignment */
int illum_af_load_identity(void)
{
    float a[10];

    /* these correspond to an identity map */    
    a[0] = 0;
    a[1] = 0;
    a[2] = 1;//(float)(1   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[3] = 0;//(float)(0   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[4] = 0;//(float)(0   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[5] = 1;//(float)(1   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[6] = 0;
    a[7] = 0;
    a[8] = 0;
    a[9] = 0;
    
    /* load the map */
    illum_af_load(a, 10);
    
    return 0;
}



/* apply the transformation equation to the cam_coords, given in subbit resolution. This equation to match
AlignmentFunctions.h: rc_[x,y] = [a0,a1] + ([[a2,a3], [a4,a5]] * (cam_[x,y] - [a6,a7]) >> PADBITS) + [a8,a9].
The additive parameters are in SUBBIT_RES.  The mult. parameters are in ALIGNMENT_PARAMS_PADBITS resolution */
int illum_af_transform_coords(IlluminateCoords_type *rel_coords, IlluminateCoords_type *cam_coords, int num_coords)
{
    int idx;
    float b0, b1, b2, b3, b4, b5;
    
    /* the redundancy in the equation can be eliminated first.  also, change everything to ALIGNMENT_PARAM_PADBITS */
    //b0 = ((al_data[0] + al_data[8]) << ALIGNMENT_PARAM_PADBITS) - al_data[2]*al_data[6] - al_data[3]*al_data[7];
    //b1 = ((al_data[1] + al_data[9]) << ALIGNMENT_PARAM_PADBITS) - al_data[4]*al_data[6] - al_data[5]*al_data[7];
    /* the redundancy in the equation can be eliminated first.  also, change everything to ALIGNMENT_PARAM_PADBITS */
    b0 = (al_data[0] + al_data[8]) - al_data[2]*al_data[6] - al_data[3]*al_data[7];
    b1 = (al_data[1] + al_data[9]) - al_data[4]*al_data[6] - al_data[5]*al_data[7];
    b2 = al_data[2];
    b3 = al_data[3];
    b4 = al_data[4];
    b5 = al_data[5];
    /* Equation is now rc_[x,y] << PADBITS = [b0,b1] + [[b2,b3], [b4,b5]] * cam_[x,y] */
    
    /* go through the coordinates, one by one */
    for(idx = 0; idx < num_coords; idx++)
    {

        //rel_coords[idx].x = (b0 + b2*cam_coords[idx].x + b3*cam_coords[idx].y + (1 << (ALIGNMENT_PARAM_PADBITS-1))) >> ALIGNMENT_PARAM_PADBITS;
        // rel_coords[idx].y = (b1 + b4*cam_coords[idx].x + b5*cam_coords[idx].y + (1 << (ALIGNMENT_PARAM_PADBITS-1))) >> ALIGNMENT_PARAM_PADBITS;
        rel_coords[idx].x = (int) (b0 \
                                    + b2*((float) cam_coords[idx].x) \
                                    + b3*((float) cam_coords[idx].y));
        rel_coords[idx].y = (int) (b1 \
                                    + b4*((float) cam_coords[idx].x) \
                                    + b5*((float) cam_coords[idx].y));
        /* The + (1 << (ALIGNMENT_PARAM_PADBITS-1)) turns the default floor() behavior into a round() */
    }

    return 0;
}



/* transform a mask specified in camera coordinates to release coordinates */
/* algorithm: transform the location of each bit in the release mask to find the nearest bit in the camera mask.
Simplified equation (should be same as in illum_transform_coord() without shift terms):
rc_[x,y] = [[a2,a3], [a4,a5]] * cam_[x,y] >> PADBITS.  This needs to be inverted:
cam_[x,y] = [[c0,c1], [c2,c3]] * rc_[x,y] >> PADBITS, where the inverse comes to:
c0 = a5/d,   c1 = -a3/d,   c2 = -a4/d,   c3 = a2/d,   d = a2*a5-a3*a4 */

int illum_af_transform_mask(IlluminateMask outmask, IlluminateMask inmask)
{
	IlluminateCoords_type cam_coord, rel_coord;
	int x, x_byte, y;
	int c0, c1, c2, c3, d;
	int cent_x, cent_y;
	int buf = 0, my_bit;
	int val;
	
	/* calculate coordinates for mask centers */
	cent_x = ILLUMINATEMASK_ROW_LEN/2*8;
	cent_y = ILLUMINATEMASK_COL_LEN/2;
    
    /* calculate the inverse matrix parameters, keeping PADBITS in mind */
	d = (al_data[2]*al_data[5] - al_data[3]*al_data[4]);
	c0 = (int) (al_data[5] / d);
	c1 = (int) (-al_data[3] / d);
	c2 = (int) (-al_data[4]  / d);
        c3 =  (int) (al_data[2] / d);
    
    /* go through each byte, then bit of outmask, equating it to the corresponding nearest bit in inmask */
    for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
    {
        rel_coord.y = y - cent_y;

        for(x_byte = 0; x_byte < ILLUMINATEMASK_ROW_LEN; x_byte++)
        {
            for(x = 0; x < 8; x++)
            {
                /* construct this byte bit-by-bit, shifting as we go */
                buf <<= 1;
                
                /* find current position in outmask, origin at center */
                rel_coord.x = x_byte*8 + x - cent_x;    /* rel_coord.y already computed above */
                
                /* find corresponding position in inmask, origin at top-left */
                /*cam_coord.x = ( (c0 * rel_coord.x + c1 * rel_coord.y + (1 << (ALIGNMENT_PARAM_PADBITS-1)) ) >> ALIGNMENT_PARAM_PADBITS ) + cent_x;
                  cam_coord.y = ( (c2 * rel_coord.x + c3 * rel_coord.y + (1 << (ALIGNMENT_PARAM_PADBITS-1)) ) >> ALIGNMENT_PARAM_PADBITS ) + cent_y;*/
                /* The + (1 << (ALIGNMENT_PARAM_PADBITS-1)) turns the default floor() behavior into a round() */
                /* trick: round +0.5 down and -0.5 up to better treat edge cases */
                val = c0 * rel_coord.x + c1 * rel_coord.y;
                if(val >= 0)
                    cam_coord.x = ( ( val + (1 << (ALIGNMENT_PARAM_PADBITS-1)) - 1 ) >> ALIGNMENT_PARAM_PADBITS ) + cent_x;
                else
                    cam_coord.x = ( ( val + (1 << (ALIGNMENT_PARAM_PADBITS-1)) - 0 ) >> ALIGNMENT_PARAM_PADBITS ) + cent_x;
                
                val = c2 * rel_coord.x + c3 * rel_coord.y;
                if(val >= 0)
                    cam_coord.y = ( ( val + (1 << (ALIGNMENT_PARAM_PADBITS-1)) - 1 ) >> ALIGNMENT_PARAM_PADBITS ) + cent_y;
                else
                    cam_coord.y = ( ( val + (1 << (ALIGNMENT_PARAM_PADBITS-1)) - 0 ) >> ALIGNMENT_PARAM_PADBITS ) + cent_y;

                
                /* check if this bit is in a valid range */
                if((cam_coord.x >= 0) && (cam_coord.x < (ILLUMINATEMASK_ROW_LEN*8)) && (cam_coord.y >= 0) && (cam_coord.y < ILLUMINATEMASK_COL_LEN))
                {
                    /* test the corresponding bit and mark it in buffer */
                    my_bit = ( inmask[cam_coord.y][cam_coord.x>>3] >> (7 - (cam_coord.x&0x07)) ) & 0x01;
                    if(my_bit)
                        buf |= 1;
                }
            }
            
            /* put the computed byte where it belongs */
            outmask[y][x_byte] = buf;
        }
    }
    
    return 0;
}



/* Analyze a camera image and identify the coordinates of an illuminated spot.  Used in determining the
camera-to-release alignment: known spot in release space needs to be mapped to spot in camera space.
Result is returned in sub-bit resolution (SUBBIT_RES); region size for statistics is determined by
IlluminateMask dimensions, with extra pixel on right and bottom to center the mask better. */
int illum_af_find_spot(unsigned short *image, IlluminateCoords_type *coords)
{
    int x, y, val;
    int max_x = 0, max_y = 0, max_val;
    long avg_x, avg_y, avg_sum;
    const int EDGE_BUFFER = 5;

    // find the brightest pixel; it should be part of the spot we're after
    max_val = -1;
    for(y = 0; y < CameraHeight; y++)
    {
        for(x = 0; x < CameraWidth; x++)
        {
            if((val = image[x + y*CameraWidth]) > max_val)
            {
                max_val = val;
                max_x = x;
                max_y = y;
            }
        }
    }
    //coords->x = max_x;
    //coords->y = max_y;
    //return 1;
    // printf("max_x: %d, max_y: %d\n", max_x, max_y);
    printf("max_val: %d\n", max_val);
    // if the max is too close to the edge for more advanced analysis, return it as the discovered coordinate
    if((max_x < EDGE_BUFFER) || (max_x > (CameraWidth - EDGE_BUFFER + 1)) || \
       (max_y < EDGE_BUFFER)   || (max_y > (CameraHeight - EDGE_BUFFER + 1)))
    {
        coords->x = max_x; //* SUBBIT_RES;
        coords->y = max_y; //* SUBBIT_RES;
        return max_val;   // indicates success but that simpler method (max_val) was used
    }
    
    // if we have space, identify our spot as the centroid of the region around max_val
    // use a region size corresponding to IlluminateMask
    avg_sum = 0;
    avg_x = 0;
    avg_y = 0;
    for(y = (max_y-EDGE_BUFFER); y < (max_y + EDGE_BUFFER + 1); y++)
    {
        for(x = (max_x-EDGE_BUFFER); x < (max_x + EDGE_BUFFER + 1); x++)
        {
            val = image[x + y*CameraWidth];
            avg_x += val * x;
            avg_y += val * y;
            avg_sum += val;
        }
    }
    avg_x = avg_x  / avg_sum;
    avg_y = avg_y / avg_sum;
    
    coords->x = (int) avg_x;
    coords->y = (int) avg_y;
    return max_val;
}


#ifdef __cplusplus
}
#endif /* __cpluspluc */

