
/* =============================================================================
// 
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// config.c: functionality for parsing a config file
//
// Release 1.0 -- 04-15-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/
#include "config.h"


/* open config file specified by filename; dies if there is
   an error
*/
void config_open(char *filename)
{
    if((config_file = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "ERROR: cannot open config file <%s>\n", filename);
        perror(NULL);
        exit(0);
    }
}


/* close the config file
 */
void config_close(void)
{
    fclose(config_file);
}


/* returns the value associated with key; if key is not present,
   returns "" as value and 0.  Otherwise returns 1
*/
int config_getvalue(char *key, char *value)
{
    char curr_line[255];
    char *curr_key;
    char *curr_value;
    char delimiter[] = "\t";
    int i;
    char log_string[255];

    *value = '\0'; /* the default return is an empty string */

    /* position ourselves at the start of the config file
     */
    _config_reset();

    /* iterate over the file; readline() returns 0 at eof
     */
    while(_config_readline(curr_line, 255))
    {

        /*ignore comments */
        if(curr_line[0]!='#')
        {

            /* remove endline character */
            for(i=0; i<255; i++)
            {
                if(*(curr_line+i) == '\n')
                {
                    *(curr_line + i) = '\0';
                    break;
                }
            }

            /* break line at tab-delimiter into key/value pair */
            curr_key = strtok(curr_line, delimiter);

            /* make sure current line is valid key\tvalue */
            if((curr_value=strtok(NULL, delimiter)) != NULL)
            {

                /* does current key match what we're looking for */
                if(!strcmp(key, curr_key))
                {
                    strcpy(value, curr_value);
                }
#ifdef DEBUG_CONFIG
                else
                {
                    sprintf(log_string, "DEBUG:\tPolonator_config: config_getvalue: did not match <%s>:<%s> to <%s>\n", curr_key, curr_value, key);
                    p_log(log_string);
                }
#endif
            }
        }
    }
    if((*value) == '\0')
    { /* key not found */
        return 0;
    }
    else
    {
        return 1;
    }
}


/* do not call externally; used to seek file pointer to start of file
 */
void _config_reset()
{
    if(fseek(config_file, SEEK_SET, 0) != 0)
    {
        p_log_errorno("ERROR:\tconfig_reset: fseek(SEEK_SET,0)");
        exit(0);
    }
    clearerr(config_file);
}

/* do not call externally; used to read next line in file
 */
int _config_readline(char *curr_line, int max_length)
{
    if(fgets(curr_line, max_length, config_file) == NULL)
    {
        if(feof(config_file))
        {
            return 0;
        }
        else
        {
            fprintf(stderr, "ERROR: _config_readline(): cannot read line\n");
            perror(NULL);
            exit(0);
        }
    }
    return 1;
}

/*
int main(int argc, char *argv[]){
  char return_val[255];
  config_open("test_cfg");
  config_getvalue("key23", return_val);
  fprintf(stdout, "returned <%s>\n", return_val);
  config_getvalue("another key", return_val);
  fprintf(stdout, "returned <%s>\n", return_val);
  config_getvalue("not_present", return_val);
  fprintf(stdout, "returned <%s>\n", return_val);
  
}
*/
