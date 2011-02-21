
/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// test_AlignmentFunctions.c: tests AlignmentFunctions.c
//
// Original version -- 08-03-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


#include <stdio.h>
#include <math.h>
#include "Release_common.h"
#include "ImageGenerator.h"
#include "AlignmentFunctions.h"
#include <G.007_acquisition/src/Polonator_logger.h>


#ifndef CAMERA_DIMENSIONS_GIVEN
#define CAMERA_IMAGE_WIDTH  1000
#define CAMERA_IMAGE_HEIGHT 1000
#endif

#ifndef IMAGEWIDTH
#define IMAGEWIDTH 24
#endif

#ifndef IMAGEHEIGHT
#define IMAGEHEIGHT 16
#endif

#define NUM_COORDS 6
#define MAX_VAL 1000
#define STD_DIV 0.6



/* Fills an image with a gaussian with given center and standard diviation */
/* coord is in sub-bit resolution */
int make_spot(CAMERA_DATA_TYPE *image, float coord_x, float coord_y, float std_div, CAMERA_DATA_TYPE max_val)
{
    int x, y;
    float x_dist, y_dist;
    
    for(y=0; y<CAMERA_IMAGE_HEIGHT; y++)
        for(x=0; x<CAMERA_IMAGE_WIDTH; x++)
        {
            x_dist = (float)x - coord_x / SUBBIT_RES;
            y_dist = (float)y - coord_y / SUBBIT_RES;
            image[x + y*CAMERA_IMAGE_WIDTH] = max_val * exp(-(x_dist*x_dist + y_dist*y_dist)/2/(std_div*std_div));
        }

    return 0;
}



int main(void)
{
    CAMERA_DATA_TYPE image[CAMERA_IMAGE_HEIGHT * CAMERA_IMAGE_WIDTH];
    float coords_x[NUM_COORDS], coords_y[NUM_COORDS];
    ReleaseCoords_type coords[NUM_COORDS], out_coords[NUM_COORDS], my_coord;
    int idx, retval;
    int a[ALIGNMENT_PARAM_NUM];
    ReleaseMask mask, out_mask;
    

    /* start the logger service */
    start_logger("./test_ImageGenerator.log",1);

    /* initialize the image generator */
    raf_init(IMAGEWIDTH, IMAGEHEIGHT, CAMERA_IMAGE_WIDTH, CAMERA_IMAGE_HEIGHT);
    
    /* set up test coordinates */
    coords_x[0] = 500    * SUBBIT_RES;    coords_y[0] = 500    * SUBBIT_RES;
    coords_x[1] = 400    * SUBBIT_RES;    coords_y[1] = 600    * SUBBIT_RES;
    coords_x[2] = 123.5  * SUBBIT_RES;    coords_y[2] = 225.5  * SUBBIT_RES;
    coords_x[3] = 477.25 * SUBBIT_RES;    coords_y[3] = 433.75 * SUBBIT_RES;
    coords_x[4] = 500.65 * SUBBIT_RES;    coords_y[4] = 500.77 * SUBBIT_RES;
    coords_x[5] = 1.65   * SUBBIT_RES;    coords_y[5] = 500.77 * SUBBIT_RES;


    /*** test raf_find_spot() ***/
    printf("Testing raf_find_spot():\n");

    /* run through the coordinates */
    for(idx=0; idx<NUM_COORDS; idx++)
    {
        printf("Coordinates given to spot generator: (%g, %g)\n", ((float)coords_x[idx]) / SUBBIT_RES, ((float)coords_y[idx]) / SUBBIT_RES);
        make_spot(image, coords_x[idx], coords_y[idx], STD_DIV, MAX_VAL);
        
        retval = raf_find_spot(image, &my_coord);
        
        printf("Coordinates found by raf_find_spot(): (%g, %g), retval = %d\n", ((float)my_coord.x) / SUBBIT_RES, ((float)my_coord.y) / SUBBIT_RES, retval);
        printf("\n");
    }
    printf("\n");
    
        
    /*** load a transform function ***/
    printf("Loading transform parameters\n");
    a[0] = 10;
    a[1] = 8;
    a[2] = (int)(0   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[3] = (int)(-2  * (1<<ALIGNMENT_PARAM_PADBITS));
    a[4] = (int)(0.5 * (1<<ALIGNMENT_PARAM_PADBITS));
    a[5] = (int)(0   * (1<<ALIGNMENT_PARAM_PADBITS));
    a[6] = CAMERA_IMAGE_WIDTH*SUBBIT_RES/2;
    a[7] = CAMERA_IMAGE_HEIGHT*SUBBIT_RES/2;
    a[8] = CAMERA_IMAGE_WIDTH*SUBBIT_RES/2;
    a[9] = CAMERA_IMAGE_HEIGHT*SUBBIT_RES/2;
    raf_load(a, ALIGNMENT_PARAM_NUM);
    printf("Paramaters: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
    printf("\n\n");


    /*** test raf_transform_coords() ***/
    printf("Testing raf_transform_coords():\n");

    /* convert the above coordinates to ReleaseCoords_type */
    for(idx=0; idx<NUM_COORDS; idx++)
    {
        coords[idx].x = (int)coords_x[idx];
        coords[idx].y = (int)coords_y[idx];
    }
    
    
    /* transform coordinates */
    raf_transform_coords(out_coords, coords, NUM_COORDS);
    for(idx=0; idx<NUM_COORDS; idx++)
        printf("Release coordinate: (%d, %d), \tcamera coordinate: (%d, %d)\n", coords[idx].x, coords[idx].y, out_coords[idx].x, out_coords[idx].y);
    printf("\n\n");


    /*** test raf_transform_mask() ***/
    printf("Testing raf_transform_mask():\n");

    /* generate a circular mask */
    printf("Input mask:\n");
    rig_mask_radius(mask, 5);
    rig_dump((unsigned char *)mask, ReleaseMask_row_len*8, ReleaseMask_col_len);
    
    /* transform mask */
    raf_transform_mask(out_mask, mask);
    printf("\nOutput mask:\n");
    rig_dump((unsigned char *)out_mask, ReleaseMask_row_len*8, ReleaseMask_col_len);

    
    return 0;
}

