#ifndef _POL_CONFIG_
#define _POL_CONFIG_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"


void config_open(char *filename);
void config_close(void);
int config_getvalue(char *key, char *value);
void _config_reset(void);
int _config_readline(char *curr_line, int max_length);

FILE *config_file;

#endif
