%module config
%{
#include "config.h"
%}

%include cpointer.i
%allocators(void, voidp);

extern void config_open(char *filename);
extern void config_close(void);
extern int config_getvalue(char *key, char *value);

