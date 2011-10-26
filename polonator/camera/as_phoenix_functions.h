#ifndef _AS_PHOENIX_FUNC_
#define _AS_PHOENIX_FUNC_

#define _PHX_LINE_SIZE     256
#define szDefaultCmdAppend "\r"

#include <phx_api.h>
#include "common/common.h"

// private functions are declared with a trailing underscore

void camera_init(void);
void camera_close(void);

int imagemean(short unsigned int* img);

void setGain(int gain);
void set_gain(tHandle hCamera, int gain);

void setExposure(double time_inseconds);
void set_exposure(tHandle hCamera, double time_inseconds);

void check_for_error(etStat eStat, char *fn_name, char *error_call);
void setupSnap(void);
int snapReceived(void);
short unsigned int* getSnapImage(void);

void startAcquire(void);
void get_buffer(void);
void release_buffer(void);
void get_buffer_cpy(unsigned short * raw_image);
short unsigned int* get_buffer_ptr(void);

void cameraInitAcq(float exposure, int gain);

void init_camera_external_trigger(tHandle hCamera);
void init_camera_internal_trigger(tHandle hCamera);
int phxser(tHandle hCamera, char *szCmdBuff);
int sPCI_readout_started(void);
void sPCI_set_readout(int startstop);
int sPCI_num_imgs(void);
int sPCI_image_ready(void);
void sPCI_set_image_ready(int ready);

/* structure to hold info from callback */
typedef struct
{
    volatile int num_imgs; /* number of images received so far */
    volatile int image_ready; /* signals a new image has been received */
    volatile int readout_started;
} tPhxCallbackInfo;

#endif
