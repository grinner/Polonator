/****************************************************************************
 *
 * ACTIVE SILICON LIMITED
 *
 * File name   : phxlive.c
 * Function    : Example Live display application
 * Project     : Phoenix
 *
 * Copyright (c) 2004-2006 Active Silicon Ltd.
 ****************************************************************************
 * Comments:
 * --------
 * This example shows how to initialise the Phoenix board and use the Display
 * library to run live double buffered (also known as ping-pong) acquisition,
 * using a callback function.
 *
 ****************************************************************************
 */

/* Define to use the Phoenix Display library */
#define _PHX_DISPLAY    

/* Application & library headers */
#include <common.h>
#include "as_phoenix_functions.h"


int phxlive(
   etCamConfigLoad eCamConfigLoad, 
   char *pszConfigFileName,          
   double exposure_time,
   int gain,
   unsigned short *frame_out 
);

int py_camera_live(double exposure_time, int gain, unsigned short *frame_out);


