#ifndef _AS_PHOENIX_FUNC_
#define _AS_PHOENIX_FUNC_

#include <phx_api.h>
#include <common.h>
#include "logger.h"

#define _PHX_LINE_SIZE     256
#define szDefaultCmdAppend "\r"


void py_cameraInit(int);
void py_cameraClose(void);
void py_set_exposure(double);
void py_set_gain(int);
int py_imagemean(short unsigned int*);
void init_camera_external_trigger(tHandle);
void init_camera_internal_trigger(tHandle);
void init_camera_internal_trigger_TDI(tHandle);
void init_camera_external_trigger_TDI(tHandle);
void set_exposure(tHandle, double);
void set_gain(tHandle, int);
void check_for_error(etStat, char *, char *);
int phxser(tHandle, char*);
void py_setupSnap(void);
int py_snapReceived(void);
short unsigned int* py_getSnapImage(void);

int sPCI_readout_started(void);
void sPCI_set_readout(int startstop);
int sPCI_num_imgs(void);
int sPCI_image_ready(void);
void sPCI_set_image_ready(int ready);
void py_startAcquire(void);
void py_get_buffer(void);
void py_release_buffer(void);
void py_get_buffer_ptr(unsigned short * raw_image);
void py_cameraInitAcq(int tdiflag, float exposure, int gain);
static void acquirer_callback(tHandle hCamera, ui32 dwInterruptMask, void *pvParams);


#endif
