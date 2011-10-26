#ifndef _AS_PHOENIX_FUNC_LIVE_
#define _AS_PHOENIX_FUNC_LIVE_

/* Define to use the Phoenix Display library */
#define _PHX_DISPLAY    

/* Application & library headers */
#include "common.h"
#include "as_phoenix_functions.h"

static void phxlive_callback( tHandle hCamera, ui32 dwInterruptMask, void *pvParams);
int phxlive(etCamConfigLoad eCamConfigLoad, char *pszConfigFileName, double exposure_time, int gain);
int camera_live(int argc, char *argv[]);

/* Define an application specific structure to hold user information */
typedef struct
{
   /* Event counters */
   volatile ui32 nBufferReadyCount;

   /* Control Flags */
   volatile tFlag fFifoOverFlow;
} tPhxLive;
#endif

int buffer_ready_count(void);

int buffer_overflow(void);