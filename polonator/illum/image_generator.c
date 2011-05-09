
/* =============================================================================
// 
// Polonator G.007 Selective Illuminate Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// ImageGenerator.c: generates images/exposure patterns for photoIlluminate of
// selected objects
//
// Original version -- 07-22-2009 [DL]
// Modified by Nick Conway 11/23/2010 removed subbit stuff
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


/* Make sure that /home/polonator/G.007 is in the include path! */


#include <stdlib.h>
#include <string.h>
/*#include <G.007_acquisition/src/Polonator_logger.h>*/
//#include "Polonator_logger.h"
#include "illuminate_common.h"
#include "image_generator.h"

/* defines to mark where the image currently is */
#define IMAGE_IN_FRAMEBUF       0
#define IMAGE_IN_SKETCHPADWIDE  2




/**************************
// private global variables
***************************/
int illum_ig_initialized = 0;    /* has system been initialized? */

/* frame buffer and computational sketch pad */
unsigned char *FrameBuf = NULL;
unsigned char *SketchPadWide = NULL;
int FrameBufSize, SketchPadWideSize;
int SketchPadWideWidth, SketchPadWideHeight;
int where_is_image = IMAGE_IN_FRAMEBUF;


HardwareData_type ImageData;



/*************************
// public global variables
**************************/




/*******************
// private functions
********************/



/* useful macros for subbit_to_wholebit() */
#define SB_BITS_MASK ((1 << SUBBIT_RES) - 1)
#define INBUF_TAB(y,x)  inbuf[x + (y)*sb_width_byte]
#define OUTBUF_TAB(y,x) outbuf[x + (y)*wb_width_byte]


/* crop the image in SketchPadWide and transfer it into FrameBuffer */
void SketchPadWide_to_FrameBuf(void)
{
    int y, pad_width, pad_height;
    
    pad_width = ImageData.Width/8;
    pad_height = ImageData.Height;
    
    for(y = 0; y < pad_height; y++)
    {
        /*printf("SketchPadIdx = %d, SketchPadWideIdx = %d\n", y * pad_width, (y+ILLUMINATEMASK_COL_LEN-1) * SketchPadWideWidth + IlluminateMask_row_len);*/
        memcpy(&(FrameBuf[y * pad_width]), \
                     &(SketchPadWide[(y+ILLUMINATEMASK_COL_LEN-1) * SketchPadWideWidth + ILLUMINATEMASK_ROW_LEN]), pad_width);
    }
}

/******************
// public functions
*******************/
#ifdef __cplusplus
extern "C"
{
#endif /* __cpluspluc */



/* initialize the module and hardware */
int illum_ig_init(HardwareData_type HardwareDescriptor)
{
    /* store the image descriptor */
    ImageData = HardwareDescriptor;

    //p_log("STATUS:\tImageGenerator: initializing");
    
    /* make sure that we can handle this device */
    if(ImageData.BitsPerPixel != 1)
    {
        //p_log("ERROR:\tImageGenerator: can only support devices with 1 bit-per-pixel");
        return -2;
    }

    /* 
    Allocate a frame buffer and sketch pad,
    but make sure that they are not already allocated 
    */
    if(FrameBuf != NULL)      free(FrameBuf);
    if(SketchPadWide != NULL) free(SketchPadWide);
    FrameBufSize = (ImageData.Width/8 * ImageData.Height); // divide by 8 since the image is in bits not bytes
    /* 
    wide sketch pad needs to be expanded by size of IlluminateMask on left and 
    right (since we pad by one in x), (IlluminateMask-1) on top and bottom.  
    This is so that shifted masks can be ORed with it directly. 
    */
    SketchPadWideWidth = ImageData.Width/8  + 2*ILLUMINATEMASK_ROW_LEN;
    SketchPadWideHeight = ImageData.Height   + 2*(ILLUMINATEMASK_COL_LEN-1);
    SketchPadWideSize =  SketchPadWideWidth * SketchPadWideHeight;
    FrameBuf      = malloc(FrameBufSize);
    SketchPadWide = malloc(SketchPadWideSize);

    if((FrameBuf == NULL) || (SketchPadWide == NULL))
    {
        //p_log("ERROR:\tImageGenerator: unable to allocate memory for frame buffers");
        return -1;
    }
  
    illum_ig_initialized = 1;
    return 0;
}



/* fill the frame buffer with the given byte fill value */
int illum_ig_fill(int fillval)
{
    /* fill the frame buffer */
    memset(FrameBuf, fillval, FrameBufSize);    /* prepare frame buffer */
    where_is_image = IMAGE_IN_FRAMEBUF;         /* mark where the image is */
    
    return 0;
}



/* 
display the image from the correct frame buffer (according to where_is_image)
returns a pointer to the frame buffer*/
unsigned char *illum_ig_render(void)
{
    /* note that each CASE cascades to the next intentionally! */
    switch(where_is_image)
    {
        case IMAGE_IN_SKETCHPADWIDE:
            SketchPadWide_to_FrameBuf();
            where_is_image = IMAGE_IN_FRAMEBUF;
            
        case IMAGE_IN_FRAMEBUF:
            return FrameBuf;
            
        default:
            return NULL;
    }
}




/* 
image construction is (OR-)cummulative, so FrameBuf must be cleared before new 
images but also clear the sketchpads, since that's where the processing will 
really happen 
*/
int illum_ig_clear_framebuffer(void)
{
    memset(FrameBuf, 0, FrameBufSize);
    memset(SketchPadWide, 0, SketchPadWideSize);

    where_is_image = IMAGE_IN_FRAMEBUF;

    return 0;
}



/* 
generate a Illuminate image corresponding to the given objects in 
Illuminate-coordinates 
MASK MUST BE BIT-BIGENDIAN (high bit to left), BYTE-LITTLENDIAN 
(earlier byte to left)!
(ie., the row 01 02 03 corresponds to the mask 000000010000001000000011) */
int illum_ig_select(IlluminateCoords_type *coords, int num_coords, IlluminateMask mask)
{
    int x, y, m, idx, temp;
    IlluminateCoords_type coord;


    /* 
    first we need to construct an array of bit-shifted masks to speed things up
    masks in the array are a byte wider than the original to allow for the 
    right-shift 
    */
    unsigned char MaskCache[8][ILLUMINATEMASK_COL_LEN][ILLUMINATEMASK_ROW_LEN+1];
    
    /* set up MaskCache[m=0] */
    for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
    {
        for(x = 0; x < ILLUMINATEMASK_ROW_LEN; x++)
        {
            MaskCache[0][y][x] = mask[y][x];
        } // end for
        MaskCache[0][y][ILLUMINATEMASK_ROW_LEN] = 0;   /* zero-out the added byte */
    } // end for
    
    /* fill in the chart */
    
    for(m = 1; m < 8; m++)
    {
        for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
        {
            MaskCache[m][y][0] = MaskCache[m-1][y][0] >> 1; // 1st byte needs special handling
            for(x = 1; x < (ILLUMINATEMASK_ROW_LEN+1); x++)
            {
                MaskCache[m][y][x] = (MaskCache[m-1][y][x-1] << 7) | (MaskCache[m-1][y][x] >> 1);
            } // end for
        } // end for
    } // end for
    /* DONE constructing MaskCache */

    
    
    /* start placing the specified objects in the sketchpad */
    for(idx = 0; idx < num_coords; idx++)
    {
        coord = coords[idx];
        
        /*printf("Receive coord = (%d, %d)\n", coord.x, coord.y); */
        
        /* check if this coord is in a range that we need to worry about */
        if((coord.x + ILLUMINATEMASK_ROW_LEN/2*8 - 1) < 0) 
        {
            continue;
        }
        if((coord.x - ILLUMINATEMASK_ROW_LEN/2*8 + 1) > (ImageData.Width))  
        {
            continue;
        }
        if((coord.y + ILLUMINATEMASK_COL_LEN/2 - 1)   < 0)
        {
            continue;
        }
        if((coord.y - ILLUMINATEMASK_COL_LEN/2 + 1)   > (ImageData.Height)) 
        {
            continue;
        }
        /*printf("Place coord = (%d, %d)\n", coord.x, coord.y);*/

        
        /* figure out this object's bit alignment (m-value in above table) */
        m = coord.x & 0x07;     /* only 3 least-signif. bits matter */
        
        /* place the corresponding mask, ORwise */
        for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
        {
            for(x = 0; x < (ILLUMINATEMASK_ROW_LEN+1); x++)
            {
                temp = (y + coord.y - ILLUMINATEMASK_COL_LEN/2 + \
                    ILLUMINATEMASK_COL_LEN -1 ) * SketchPadWideWidth + \
                    x + (coord.x >> 3) - ILLUMINATEMASK_ROW_LEN/2 + \
                    ILLUMINATEMASK_ROW_LEN;
                SketchPadWide[temp] |= MaskCache[m][y][x];
            } // end for
        } // end for
    }
    
    return where_is_image = IMAGE_IN_SKETCHPADWIDE;    /* mark where the image is */
}



/* 
generate a circular Illuminate mask by specifying radius.  
Units are in SUBBIT_RES resolution. 
*/
int illum_ig_mask_radius(IlluminateMask mask, int rad)
{
    int x, y;
    int cent_x, cent_y;
    int rad2; /* radius squared */
    unsigned char this_byte;
    
    /* calculate coordinates of mask center relative to upper left corner, in bits */
    cent_x = ILLUMINATEMASK_ROW_LEN/2*8;
    cent_y = ILLUMINATEMASK_COL_LEN/2;
    
    this_byte = 0;
    rad2 = (rad * rad);
    
    for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
    {
        for(x = 0; x < ( ILLUMINATEMASK_ROW_LEN*8); x++)    /* x is counting bits, not bytes! */
        {
            this_byte <<= 1;
            
            /* check if current bit is within the radius */
            if( ((x-cent_x)*(x-cent_x) + (y-cent_y)*(y-cent_y)) <= rad2 )
            {
                this_byte |= 1;     /* mark this byte as on */
            }
            /* check if we're at the end of a byte */
            if( (x & 0x07) == 7 )
            {
                mask[y][x >> 3] = this_byte;    /* store the current byte */
            }
        }
    }
    return 0;
}



/* A diagnostic function for displaying bit images text.  Width given in bits, 
not bytes 
*/
void illum_ig_dump(unsigned char *image, int Width, int Height)
{
#ifdef WITH_ILLUM_IG_DUMP
    int x, y, x_byte;
    unsigned char this_char;
    
    printf("Width = %d, Height = %d\n", Width, Height);   /* start with an empty row */
    
    for(y = 0; y < Height; y++)     /* go through rows */
    {
        /*printf("y = %d:\t", y);*/
        for( x_byte = 0; x_byte < (Width/8); x_byte++)
        {
            this_char = image[y * (Width/8) + x_byte];
            
            for(x = 0; x < 8; x++)
            {
                if(this_char & 0x80)
                    printf("O");        /* big symbol if a 1 bit */
                else
                    printf(".");        /* small symbol if a 0 bit */
                    
                this_char <<= 1;        /* shift to next bit */
            }
        }
        
        printf("\n");   /* cap off a row */
    }
    
    printf("\n");   /* add an empty row at the end */
#endif /* WITH_ILLUM_IG_DUMP */
}


/*
generate a Illuminate image corresponding to the given objects in 
Illuminate-coordinates MASK MUST BE BIT-BIGENDIAN (high bit to left), 
BYTE-LITTLENDIAN (earlier byte to left)! (ie., the row 01 02 03 corresponds to 
the mask 000000010000001000000011) 
*/
int illum_ig_points(int *vx, int *vy, int num_coords, IlluminateMask mask)
{
    int x, y, m, idx. temp;

    /* 
    first we need to construct an array of bit-shifted masks to speed things up 
    masks in the array are a byte wider than the original to allow for the 
    right-shift 
    */
    unsigned char MaskCache[8][ILLUMINATEMASK_COL_LEN][ILLUMINATEMASK_ROW_LEN+1];
    
    /* set up MaskCache[m=0] */
    for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
    {
        for(x = 0; x < ILLUMINATEMASK_ROW_LEN; x++)
        {
            MaskCache[0][y][x] = mask[y][x];
        }
        MaskCache[0][y][ILLUMINATEMASK_ROW_LEN] = 0;   /* zero-out the added byte */
    }
    
    /* fill in the chart */
    
    for(m = 1; m < 8; m++)
    {
        for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
        {
            MaskCache[m][y][0] = MaskCache[m-1][y][0] >> 1; // 1st byte needs special handling
            for(x = 1; x < (ILLUMINATEMASK_ROW_LEN+1); x++)
            {
                MaskCache[m][y][x] = (MaskCache[m-1][y][x-1] << 7) | (MaskCache[m-1][y][x] >> 1);
            }
        }
    }
    /* DONE constructing MaskCache */

    
    /* start placing the specified objects in the sketchpad */
    for(idx = 0; idx < num_coords; idx++)
    {
        /*printf("Receive coord = (%d, %d)\n", vx[idx], vy[idx]); */
        
        /* check if this coord is in a range that we need to worry about */
        if((vx[idx] + ILLUMINATEMASK_ROW_LEN/2*8 - 1) < 0)                  continue;
        if((vx[idx] - ILLUMINATEMASK_ROW_LEN/2*8 + 1) > (ImageData.Width))  continue;
        if((vy[idx] + ILLUMINATEMASK_COL_LEN/2 - 1)   < 0)                  continue;
        if((vy[idx] - ILLUMINATEMASK_COL_LEN/2 + 1)   > (ImageData.Height)) continue;

        /*printf("Place coord = (%d, %d)\n", vx[idx], vy[idx]);*/

        
        /* figure out this object's bit alignment (m-value in above table) */
        m = vx[idx] & 0x07;     /* only 3 least-signif. bits matter */
        
        /* place the corresponding mask, ORwise */
        for(y = 0; y < ILLUMINATEMASK_COL_LEN; y++)
        {
            for(x = 0; x< (ILLUMINATEMASK_ROW_LEN+1); x++)
            {
                temp = (y + vy[idx] - ILLUMINATEMASK_COL_LEN/2 + \
                    ILLUMINATEMASK_COL_LEN -1 ) * SketchPadWideWidth + \
                    x + (vx[idx] >> 3) - ILLUMINATEMASK_ROW_LEN/2 + \
                    ILLUMINATEMASK_ROW_LEN;
                SketchPadWide[temp] |= MaskCache[m][y][x];
            }
        }
    }
    
    return where_is_image = IMAGE_IN_SKETCHPADWIDE;    /* mark where the image is */
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


