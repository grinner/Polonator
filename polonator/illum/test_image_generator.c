
/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// test_ImageGenerator.c: tests ImageGenerator.c
//
// Original version -- 07-27-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


#include <stdio.h>
#include "Release_common.h"
#include "ImageGenerator.h"
#include <G.007_acquisition/src/Polonator_logger.h>


#define COORDS_NUM 7


#ifndef IMAGEWIDTH
#define IMAGEWIDTH 24
#endif

#ifndef IMAGEHEIGHT
#define IMAGEHEIGHT 16
#endif

#ifdef REPEATS
#define REPEATS_NUM 500
#endif
 


/* enable access to ImageGenerator internals */
extern unsigned char *SketchPad;
extern unsigned char *SketchPadWide;
extern int SketchPadWideWidth;
extern int SketchPadWideHeight;


HardwareData_type my_hardware;
ReleaseMask my_mask;




int main(void)
{
    unsigned char *my_framebuf;
    ReleaseCoords_type coords[COORDS_NUM];
    int idx;

    
    /* define the size of the images generated */
    my_hardware.Width = IMAGEWIDTH;
    my_hardware.Height = IMAGEHEIGHT;
    my_hardware.BitsPerPixel = 1;
    
    /* start the logger service */
    start_logger("./test_ImageGenerator.log",1);

    /* initialize the image generator */
    rig_init(my_hardware);
    
    /* generate a mask of radius = 6 subbit-units */
    rig_mask_radius(my_mask, 6);
    rig_dump((unsigned char *)my_mask, ReleaseMask_row_len*8, ReleaseMask_col_len);
    
    /* generate coordinates for testing rig_selec() */
    for(idx=0; idx<COORDS_NUM; idx++)
    {
        coords[idx].x = 0 + idx*13;
        coords[idx].y = 0 + idx*13;
    }

#ifdef REPEATS
    for(idx=0; idx<REPEATS_NUM; idx++)
    {
#endif
    
    /* clear the frame buffers */
    rig_clear_framebuffer();
    
    /* run rig_select() on generated coordinates */
    rig_select(coords, COORDS_NUM, my_mask);
    
    /* render down to regular resolution and show */ 
    my_framebuf = rig_render();

#ifdef REPEATS
}
#else

    /*printf("SketchPadWide:\n");
    rig_dump(SketchPadWide, SketchPadWideWidth*8, SketchPadWideHeight); */
    printf("SketchPad:\n");
    rig_dump(SketchPad, my_hardware.Width*SUBBIT_RES, my_hardware.Height*SUBBIT_RES);
    printf("FrameBuf:\n");
    rig_dump(my_framebuf, my_hardware.Width, my_hardware.Height);

#endif /* REPEATS */

    return 0;
}

