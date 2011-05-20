%module logger
%{
#include "logger.h"
%}
/* Wyss Institute Polonator_logger.i SWIG interface file */
/* Version 1.0 Nick Conway 1/20/2010 */

%include logger.h
%include cpointer.i
%allocators(void, voidp);

extern void p_log(char*);
extern void p_log_errorno(char *message);
extern void p_log_simple(char*);
extern void start_logger(char*, int);

