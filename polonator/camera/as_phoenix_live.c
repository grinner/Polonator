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

  /* This is the modified version of active silicon source by Nick Conway*/
 
/* Define to use the Phoenix Display library */
#define _PHX_DISPLAY    
// #define _USE_QT

/* Application & library headers */

#include <stdio.h>

#include <common.h>
#include "as_phoenix_functions.h"
//#include <phx_api.h>
#include "as_phoenix_live.h"


/*
phxlive_callback(ForBrief)
This is the callback function which handles the interrupt events
*/
tPhxLive       sPhxLive;            /* User defined Event Context */

static void phxlive_callback(
    tHandle hCamera,        /* Camera handle. */
    ui32 dwInterruptMask,   /* Interrupt mask. */
    void *pvParams          /* Pointer to user supplied context */
) 
{
    tPhxLive *psPhxLive = (tPhxLive*) pvParams;

    (void) hCamera;

    /* Handle the Buffer Ready event */
    if ( PHX_INTRPT_BUFFER_READY & dwInterruptMask ) 
    {
      /* Increment the Display Buffer Ready Count */
      psPhxLive->nBufferReadyCount++;
    }

    /* Fifo Overflow */
    if ( PHX_INTRPT_FIFO_OVERFLOW & dwInterruptMask ) 
    {
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
Simple live capture application code
*/
int phxlive(
    etCamConfigLoad eCamConfigLoad,    /* Board number, ie 1, 2, or 0 for next available */
    char *pszConfigFileName,            /* Name of config file */
    double exposure_time,
    int gain,
    unsigned short *frame_out          // added by NC to allow output to another window
)
{
    etStat         eStat     = PHX_OK;  /* Status variable */
    etParamValue   eParamValue;         /* Parameter for use with PHX_ParameterSet/Get calls */
    tHandle        hCamera   = 0;       /* Camera Handle  */
    tPHX           hDisplay  = 0;       /* Display handle */
    tPHX           hBuffer1  = 0;       /* First Image buffer handle  */
    tPHX           hBuffer2  = 0;       /* Second Image buffer handle */
    //tPhxLive       sPhxLive;            /* User defined Event Context */
    ui32           nBufferReadyLast = 0;/* Previous BufferReady count value */
    int i,length;

    /* Initialise the user defined Event context structure */
    memset( &sPhxLive, 0, sizeof( tPhxLive ) );

    /* Allocate the board with the config file */
    eStat = PHX_CameraConfigLoad( &hCamera, pszConfigFileName, eCamConfigLoad, PHX_ErrHandlerDefault );
    if ( PHX_OK != eStat ) goto Error;

    /* set camera to live acquisition mode */
    init_camera_internal_trigger(hCamera);
    set_exposure(hCamera, exposure_time);
    set_gain(hCamera, gain);


#ifndef _USE_QT
    // We create our display with a NULL hWnd, this will automatically create an image window. 
    eStat = PDL_DisplayCreate( &hDisplay, NULL, hCamera, PHX_ErrHandlerDefault );
    if ( PHX_OK != eStat ) goto Error;

    // We create two display buffers for our double buffering 
    eStat = PDL_BufferCreate( &hBuffer1, hDisplay, (etBufferMode)PDL_BUFF_SYSTEM_MEM_DIRECT );
    if ( PHX_OK != eStat ) goto Error;
    eStat = PDL_BufferCreate( &hBuffer2, hDisplay, (etBufferMode)PDL_BUFF_SYSTEM_MEM_DIRECT );
    if ( PHX_OK != eStat ) goto Error;

    // Initialise the display, this associates the display buffers with the display 
    eStat =  PDL_DisplayInit( hDisplay );
    if ( PHX_OK != eStat ) goto Error;

    // The above code has created 2 display (acquisition) buffers.
    // Therefore ensure that the Phoenix is configured to use 2 buffers, by overwriting
    // the value already loaded from the config file.
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
    //printf("Press a key to exit\n");
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
        if ( nBufferReadyLast != sPhxLive.nBufferReadyCount ) 
        {
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
#ifndef _USE_QT
            PDL_BufferPaint( (tPHX)stBuffer.pvContext );
#elif defined _USE_QT
            // Load a numpy array here!!!!
            for(i = 0; i < 1000000; i++)
            {
                *(frame_out + i) = *((short unsigned int*)(stBuffer.pvAddress) + i);
            }
            //fflush(stdout);
            //length = sizeof(frame_out);
            //write(1, frame_out, length);

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
    {
      printf("FIFO OverFlow detected..Aborting\n");
    }
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

int buffer_ready_count(void)
{
    return sPhxLive.nBufferReadyCount;
}

int buffer_overflow(void)
{
    return sPhxLive.fFifoOverFlow;
}


int py_camera_live(double exposure_time, int gain, unsigned short *frame_out)
{
    tPhxCmd sPhxCmd;
    int     nStatus;
    sPhxCmd.dwBoardNumber     = 1; 
    sPhxCmd.pszConfigFileName = NULL;
    sPhxCmd.pszOutputFileName = NULL;
    sPhxCmd.dwBayerOption     = 11;
    sPhxCmd.dwGammaOption     = 100;
    sPhxCmd.dwFrameOption     = 300;
    sPhxCmd.dwTimeOption      = 3;
    sPhxCmd.dwSlowOption      = 10;
    sPhxCmd.eCamConfigLoad = (etCamConfigLoad) ( PHX_DIGITAL | sPhxCmd.dwBoardNumber );

    char filepath_buffer[256];
    strcpy(filepath_buffer, getenv("POLONATOR_PATH"));
   
    /*PhxCommonKbInit();*/
    strcat(filepath_buffer, "/config_files/em9100-02.pcf");
    nStatus = phxlive( sPhxCmd.eCamConfigLoad, \
                    filepath_buffer, \
                    exposure_time, \
                    gain, \
                    frame_out);
    /*PhxCommonKbClose();*/
    return nStatus;
}

