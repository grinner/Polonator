#ifndef _POL_LOGGER
#define _POL_LOGGER

void start_logger(char *logfilename, int df);
void close_logger(void);
void p_log(char *message);
void p_log_simple(char *message);
void p_log_errorno(char *message);
void set_disp(int d);

#endif
