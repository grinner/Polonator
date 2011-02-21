%module AS_PhoenixFunctions
%{
#include "as_phoenix_functions.h"
%}


%include as_phoenix_functions.h
%include logger.h
%include cpointer.i
%allocators(void, voidp);

extern void py_cameraInit(int);
extern void py_cameraClose(void);
extern void py_set_exposure(double);
extern void py_set_gain(int);
extern short unsigned int* py_snapimage(void);
extern int py_imagemean(short unsigned int*);
