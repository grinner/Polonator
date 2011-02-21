
/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// hardware_D4000.c: hardware functions for D4000 DMD array using an API that
// allows overloading support for other hardware using #ifdefs
//
// Original version -- 07-22-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


/* Assumes that Polonator_logger has been initialized!!! */
/* Make sure that /home/polonator/G.007 is in the include path! */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libD4000.h>
//#include "libD4000/src/libD4000.h"
//#include <G.007_acquisition/src/Polonator_logger.h>
#include "logger.h"

#include "illuminate_common.h"
//#include "Polonator_IlluminateFunctions.h"
#include "hardware_D4000.h"




/* global variables */
int rhw_enabled_state;
unsigned char *FrameBuf_off = NULL;     /* frame buffer for holding disabled-state image */
int FrameBuf_off_size;



/* first-run initialization of hardware and libraries */
int rhw_init(HardwareData_type *HardwareDescriptor)
{
    int retval;
    DeviceData_type DeviceData;
    char message[200];
    
    p_log("STATUS:\thardware_D4000: initializing");
    
    /* initialize libD4000 and make sure a device is connected */
    retval = libD4000_init();
    if(!libD4000_IsDeviceAttached(DEVNUM))
    {
        p_log("ERROR:\thardware_D4000: DMD not found");
        return -1;
    }

    /* connect to the device and upload firmware if needed */
    if(libD4000_ConnectDevice(DEVNUM, D4000_USB_BIN) < 0)
    {
        sprintf(message, "ERROR:\thardware_D4000: Could not connect to device.  Check firmware file %s", D4000_USB_BIN);
        p_log(message);
        return -2;
    }

    /* get device data (DMD size, etc.) */
    DeviceData = libD4000_GetDeviceData(DEVNUM);
    HardwareDescriptor->Width = DeviceData.DMDWidth;
    HardwareDescriptor->Height = DeviceData.DMDHeight;
    HardwareDescriptor->BitsPerPixel = DeviceData.DMDSizeinBytes*8/(DeviceData.DMDWidth*DeviceData.DMDHeight);

    /* setup FrameBuf_off -- holds image for disabled state */
    if(FrameBuf_off != NULL) 
    {	
    	free(FrameBuf_off);
        printf("weird!!!!!");
    }
    FrameBuf_off_size = DeviceData.DMDSizeinBytes;
    if((FrameBuf_off = malloc(FrameBuf_off_size)) == NULL)
    {
        p_log("ERROR:\thardware_D4000: Error allocating frame buffer");
        return -3;
    }
    memset(FrameBuf_off, RHW_DISABLE_FILL, FrameBuf_off_size);    /* prepare frame buffer */
    libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
    //libD4000_GetRESETCOMPLETE(5000, DEVNUM);
    //libD4000_FloatMirrors(DEVNUM);
    if (libD4000_GetDMDTYPE(DEVNUM) == 0)
    {
        printf("awesome!!!!!");
    }

    return retval;  
}

int rhw_light_on(void)
{
     DeviceData_type DeviceData = libD4000_GetDeviceData(DEVNUM);
    /* setup FrameBuf_off -- holds image for disabled state */
    if(FrameBuf_off != NULL)
    {
    	free(FrameBuf_off);
    }
    FrameBuf_off_size = DeviceData.DMDSizeinBytes;
    if((FrameBuf_off = malloc(FrameBuf_off_size)) == NULL)
    {
        p_log("ERROR:\thardware_D4000: Error allocating frame buffer");
        return -3;
    }
    memset(FrameBuf_off, RHW_DISABLE_FILL, FrameBuf_off_size);    /* prepare frame buffer */
    return 0;
}

int rhw_light_off(void)
{
     DeviceData_type DeviceData = libD4000_GetDeviceData(DEVNUM);
    /* setup FrameBuf_off -- holds image for disabled state */
    if(FrameBuf_off != NULL)
    {
    	free(FrameBuf_off);
    }
    FrameBuf_off_size = DeviceData.DMDSizeinBytes;
    if((FrameBuf_off = malloc(FrameBuf_off_size)) == NULL)
    {
        p_log("ERROR:\thardware_D4000: Error allocating frame buffer");
        return -3;
    }
    memset(FrameBuf_off, 0, FrameBuf_off_size);    /* prepare frame buffer */
    //libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
    return 0;
}

/* hardware-specific command to run before regular imaging */
int rhw_disable(void)
{
    /* display an all-one image */
    /*rhw_display(FrameBuf_off);  not good here because rhw_enabled_state may == 0!! */
    libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
    libD4000_LoadToDMD(FrameBuf_off, DMD_ALLBLOCKS, 1, DEVNUM);
    //libD4000_LoadToDMD(FrameBuf_off, DMD_ALLBLOCKS, 1, DEVNUM);
    //libD4000_GetRESETCOMPLETE(500, DEVNUM);
    rhw_enabled_state = 0;
    return 0;
}



/* hardware-specific command to start release exposure */
int rhw_enable(void)
{
    /* nothing to do */
    rhw_enabled_state = 1;
    return 0;
}


/* hardware-specific command to start release exposure */
int rhw_float(void)
{
    /* nothing to do */
    libD4000_FloatMirrors(DEVNUM);
    return 0;
}

int rhw_clear_mem(void)
{
    libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
    return 0;
}

/* display the given frame on the DMD, but respect rhw_enabled_state */
int rhw_load_image(unsigned char *frame)
{
    if(rhw_enabled_state)
    {
    	//printf("Fault coming up...\n");
        //libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
        //libD4000_LoadToDMD(frame, DMD_ALLBLOCKS, 1, DEVNUM);
        libD4000_Clear(DMD_ALLBLOCKS, 1, DEVNUM);
        return libD4000_LoadToDMD(frame, DMD_ALLBLOCKS, 1, DEVNUM);
    }
    else
        return 1;
}

