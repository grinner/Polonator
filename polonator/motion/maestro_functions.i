%module maestroFunctions
%{
#include "maestro_functions.h"
%}
/* Wyss Institute maestro_functions.i SWIG interface file */
/* Version 1.0 Nick Conway 1/20/2010 */


%include maestro_functions.h
%include logger.h
%include cpointer.i
%allocators(void, voidp);

extern void py_maestro_open(char*, int);
extern void py_maestro_setupimaging(int, int, int, int, int, int, int);
extern void py_maestro_setcolor(char*);
extern void py_maestro_unlock(void);
extern void py_maestro_lock(void);
extern void py_maestro_shutter_open(void);
extern void py_maestro_shutter_close(void);
extern void py_maestro_darkfield_on(void);
extern void py_maestro_darkfield_off(void);
extern void py_maestro_goto_image(int, int, int);
extern void py_maestro_hometheta(void);
extern void py_maestro_unlocktheta(void);
extern void py_maestro_locktheta(void);
