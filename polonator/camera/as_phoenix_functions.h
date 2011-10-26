#ifndef _AS_PHOENIX_FUNC_
#define _AS_PHOENIX_FUNC_

#define _PHX_LINE_SIZE     256
#define szDefaultCmdAppend "\r"

/* structure to hold info from callback */
typedef struct
{
    volatile int num_imgs; /* number of images received so far */
    volatile int image_ready; /* signals a new image has been received */
    volatile int readout_started;
} tPhxCallbackInfo;


// private functions are declared with a trailing underscore

void camera_init(void);
void camera_close(void);

int imagemean(short unsigned int*);

void setGain(int);
static void set_gain(tHandle, int);

void setExposure(double time_inseconds);
static void set_exposure(tHandle, double time_inseconds);

void check_for_error(etStat, char *, char *);
void setupSnap(void);
int snapReceived(void);
short unsigned int* getSnapImage(void);

void startAcquire(void);
void get_buffer(void);
void release_buffer(void);
void get_buffer_cpy(unsigned short * raw_image);
short unsigned int* get_buffer_ptr(void);

void cameraInitAcq(float exposure, int gain);

static void acquirer_callback(tHandle hCamera, ui32 dwInterruptMask, void *pvParams);

static void init_camera_external_trigger(tHandle);
static void init_camera_internal_trigger(tHandle);
static int phxser_(tHandle, char*);
static int sPCI_readout_started(void);
static void sPCI_set_readout(int startstop);
static int sPCI_num_imgs(void);
static int sPCI_image_ready(void);
static void sPCI_set_image_ready(int ready);

#endif
