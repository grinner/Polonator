%module asPhoenixFunctions
%{
#include "as_phoenix_functions.h"
%}

%include "logger.h"
%include "as_phoenix_functions.h"
%include cpointer.i
%allocators(void, voidp);

extern void camera_init(void);
extern void camera_initAcq(void);
extern void camera_close(void);
extern void setExposure(double);
extern void setGain(int);
extern short unsigned int* snapimage(void);
extern int imagemean(short unsigned int*);

extern short unsigned int* get_buffer_ptr(void);

%include "numpy.i"
  
%init %{
    import_array();
%}

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * raw_image) }
%rename (get_buffer_cpy) my_get_buffer_cpy;

%inline %{
    void my_get_buffer_cpy(int len1, unsigned short * raw_image)
    {
       get_buffer_cpy(unsigned short * raw_image);
    }

%}

extern int sPCI_readout_started(void);
extern void sPCI_set_readout(int startstop);
extern int sPCI_num_imgs(void);
extern int sPCI_image_ready(void);
extern void sPCI_set_image_ready(int ready);
extern void startAcquire(void);
extern void get_buffer(void);
extern void release_buffer(void);
extern void camera_initAcq(float exposure, int gain);

// Live prototypes
%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len2, unsigned short * frame_out) }

