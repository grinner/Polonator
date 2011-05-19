%module AS_PhoenixFunctions
%{
#include "as_phoenix_functions.h"
%}


%include as_phoenix_functions.h
%include logger.h
%include cpointer.i
%allocators(void, voidp);

extern void py_cameraInit(int);
extern void py_cameraInitAcq(int tdiflag);
extern void py_cameraClose(void);
extern void py_set_exposure(double);
extern void py_set_gain(int);
extern short unsigned int* py_snapimage(void);
extern int py_imagemean(short unsigned int*);


%include "numpy.i"
  
%init %{
    import_array();
%}

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * raw_image) }
%rename (py_get_buffer_ptr) my_py_get_buffer_ptr;

%inline %{
    void my_py_get_buffer_ptr(int len1, unsigned short * raw_image)
    {
       py_get_buffer_ptr(unsigned short * raw_image);
    }

%}

extern int sPCI_readout_started(void);
extern void sPCI_set_readout(int startstop);
extern int sPCI_num_imgs(void);
extern int sPCI_image_ready(void);
extern void sPCI_set_image_ready(int ready);
extern void py_startAcquire(void);
extern void py_get_buffer(void);
extern void py_release_buffer(void);
extern void py_cameraInitAcq(int tdiflag, float exposure, int gain);

