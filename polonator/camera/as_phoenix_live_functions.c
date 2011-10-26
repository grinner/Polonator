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

 
 /* This is the unmodified version from active silicon */
 
 
/* Define to use the Phoenix Display library */
#define _PHX_DISPLAY    

/* Application & library headers */
#include <common.h>
#include "as_phoenix_functions.h"

/* Define an application specific structure to hold user information */
typedef struct
{
   /* Event counters */
   volatile ui32 nBufferReadyCount;

   /* Control Flags */
   volatile tFlag fFifoOverFlow;
} tPhxLive;


/*
phxlive_callback(ForBrief)
 * This is the callback function which handles the interrupt events
 */
static void phxlive_callback(
   tHandle hCamera,        /* Camera handle. */
   ui32 dwInterruptMask,   /* Interrupt mask. */
   void *pvParams          /* Pointer to user supplied context */
) 
{
   tPhxLive *psPhxLive = (tPhxLive*) pvParams;

   (void) hCamera;

   /* Handle the Buffer Ready event */
   if ( PHX_INTRPT_BUFFER_READY & dwInterruptMask ) {
      /* Increment the Display Buffer Ready Count */
      psPhxLive->nBufferReadyCount++;
   }

   /* Fifo Overflow */
   if ( PHX_INTRPT_FIFO_OVERFLOW & dwInterruptMask ) {
      psPhxLive->fFifoOverFlow = TRUE;
   }

   /* Note:
    * The callback routine may be called with more than 1 event flag set.
    * Therefore all possible events must be handled here.
    */
   if ( PHX_INTRPT_FRAME_END & dwInterruptMask )
   {
   }
}




/*
phxlive(ForBrief)
 * Simple live capture application code
 */
int phxlive(
   etCamConfigLoad eCamConfigLoad,    /* Board number, ie 1, 2, or 0 for next available */
   char *pszConfigFileName,            /* Name of config file */
   double exposure_time,
   int gain
)
{
   etStat         eStat     = PHX_OK;  /* Status variable */
   etParamValue   eParamValue;         /* Parameter for use with PHX_ParameterSet/Get calls */
   tHandle        hCamera   = 0;       /* Camera Handle  */
   tPHX           hDisplay  = 0;       /* Display handle */
   tPHX           hBuffer1  = 0;       /* First Image buffer handle  */
   tPHX           hBuffer2  = 0;       /* Second Image buffer handle */
   tPhxLive       sPhxLive;            /* User defined Event Context */
   ui32           nBufferReadyLast = 0;/* Previous BufferReady count value */


   /* Initialise the user defined Event context structure */
   memset( &sPhxLive, 0, sizeof( tPhxLive ) );

   /* Allocate the board with the config file */
   eStat = PHX_CameraConfigLoad( &hCamera, pszConfigFileName, eCamConfigLoad, PHX_ErrHandlerDefault );
   if ( PHX_OK != eStat ) goto Error;

   /* set camera to live acquisition mode */
   init_camera_internal_trigger(hCamera);
   set_exposure(hCamera, exposure_time);
   set_gain(hCamera, gain);

#if defined _PHX_DISPLAY
   /* We create our display with a NULL hWnd, this will automatically create an image window. */
   eStat = PDL_DisplayCreate( &hDisplay, NULL, hCamera, PHX_ErrHandlerDefault );
   if ( PHX_OK != eStat ) goto Error;

   /* We create two display buffers for our double buffering */
   eStat = PDL_BufferCreate( &hBuffer1, hDisplay, (etBufferMode)PDL_BUFF_SYSTEM_MEM_DIRECT );
   if ( PHX_OK != eStat ) goto Error;
   eStat = PDL_BufferCreate( &hBuffer2, hDisplay, (etBufferMode)PDL_BUFF_SYSTEM_MEM_DIRECT );
   if ( PHX_OK != eStat ) goto Error;

   /* Initialise the display, this associates the display buffers with the display */
   eStat =  PDL_DisplayInit( hDisplay );
   if ( PHX_OK != eStat ) goto Error;

   /* The above code has created 2 display (acquisition) buffers.
    * Therefore ensure that the Phoenix is configured to use 2 buffers, by overwriting
    * the value already loaded from the config file.
    */
   eParamValue = (etParamValue) 2;
   eStat = PHX_ParameterSet( hCamera, PHX_ACQ_NUM_IMAGES, &eParamValue );
   if ( PHX_OK != eStat ) goto Error;
#endif


   /* Enable FIFO Overflow events */
   eParamValue = PHX_INTRPT_FIFO_OVERFLOW;
   eStat = PHX_ParameterSet( hCamera, PHX_INTRPT_SET, &eParamValue );
   if ( PHX_OK != eStat ) goto Error;

   /* Setup our own event context */
   eStat = PHX_ParameterSet( hCamera, PHX_EVENT_CONTEXT, (void *) &sPhxLive );
   if ( PHX_OK != eStat ) goto Error;


   /* Now start our capture, using the callback method */
   eStat = PHX_Acquire( hCamera, PHX_START, (void*) phxlive_callback );
   if ( PHX_OK != eStat ) goto Error;


   /* Continue processing data until the user presses a key in the console window
    * or Phoenix detects a FIFO overflow
    */
   printf("Press a key to exit\n");
   /*   while(!PhxCommonKbHit() && !sPhxLive.fFifoOverFlow)*/
   while(!sPhxLive.fFifoOverFlow)
   {
      /* Temporarily sleep, to avoid burning CPU cycles.
       * An alternative method is to wait on a semaphore, which is signalled
       * within the callback function.  This approach would ensure that the
       * data processing would only start when there was data to process
       */
      _PHX_SleepMs(10);

      /* If there are any buffers waiting to display, then process them here */
      if ( nBufferReadyLast != sPhxLive.nBufferReadyCount ) {
         stImageBuff stBuffer; 
         int nStaleBufferCount;

         /* If the processing is too slow to keep up with acquisition,
          * then there may be more than 1 buffer ready to process.
          * The application can either be designed to process all buffers
          * knowing that it will catch up, or as here, throw away all but the
          * latest
          */
         nStaleBufferCount = sPhxLive.nBufferReadyCount - nBufferReadyLast;
         nBufferReadyLast += nStaleBufferCount;

         /* Throw away all but the last image */
         while ( nStaleBufferCount-- > 1 )
         {
            eStat = PHX_Acquire( hCamera, PHX_BUFFER_RELEASE, NULL );
            if ( PHX_OK != eStat ) goto Error;
         }


         /* Get the info for the last acquired buffer */
         eStat = PHX_Acquire( hCamera, PHX_BUFFER_GET, &stBuffer );
         if ( PHX_OK != eStat ) goto Error;

         /* Process the newly acquired buffer,
          * which in this simple example is a call to display the data.
          * For our display function we use the pvContext member variable to
          * pass a display buffer handle.
          * Alternatively the actual video data can be accessed at stBuffer.pvAddress
          */
#if defined _PHX_DISPLAY
         PDL_BufferPaint( (tPHX)stBuffer.pvContext );
#else
         printf("EventCount = %5d\r", sPhxLive.nBufferReadyCount );
#endif

         /* Having processed the data, release the buffer ready for further image data */
         eStat = PHX_Acquire( hCamera, PHX_BUFFER_RELEASE, NULL );
         if ( PHX_OK != eStat ) goto Error;
      }
   }
   printf("\n");


   /* In this simple example we abort the processing loop on an error condition (FIFO overflow).
    * However handling of this condition is application specific, and generally would involve
    * aborting the current acquisition, and then restarting.
    */
   if ( sPhxLive.fFifoOverFlow )
      printf("FIFO OverFlow detected..Aborting\n");

Error:
   /* Now cease all captures */
   if ( hCamera ) PHX_Acquire( hCamera, PHX_ABORT, NULL );

#if defined _PHX_DISPLAY
   /* Free our display double buffering resources */
   if ( hBuffer1 ) PDL_BufferDestroy( (tPHX*) &hBuffer1 );
   if ( hBuffer2 ) PDL_BufferDestroy( (tPHX*) &hBuffer2 );

   /* Destroy our display */
   if ( hDisplay ) PDL_DisplayDestroy( (tPHX*) &hDisplay );
#endif

   /* Release the Phoenix board */
   if ( hCamera ) PHX_CameraRelease( &hCamera );

   printf("Exiting\n");
   return 0;
}


/* assumes call by PolonatorUtils, which means that
   integration time in seconds is in argv[2], and EM
   gain is in argv[3]
*/
int camera_live(int argc, char *argv[])
{
    tPhxCmd sPhxCmd;
    int     nStatus;
    
    // I don't think this lineis necessary, NC 11-2011, but whatever
    PhxCommonParseCmd( argc, argv, &sPhxCmd );

    char filepath_buffer[256];
    strcpy(filepath_buffer, getenv("POLONATOR_PATH"));

    /*PhxCommonKbInit();*/
    strcat(filepath_buffer, "/config_files/em9100-02.pcf");
    nStatus = phxlive( sPhxCmd.eCamConfigLoad, filepath_buffer, atof(argv[2]), atoi(argv[3]));
    /*PhxCommonKbClose();*/
    return nStatus;
}

int buffer_ready_count(void)
{
    return sPhxLive.nBufferReadyCount;
}

int buffer_overflow(void)
{
    return sPhxLive.fFifoOverFlow;
}
