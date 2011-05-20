%module networkG007

%{
    #define SWIG_FILE_WITH_INIT
    #include "network_functions.h"
%}

%include "numpy.i"

%init %{
    import_array();
%}

%include "network_functions.h"

%rename (py_network_sendimage) my_py_network_sendimage;

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * image_ptr) }

%inline %{
    int my_py_network_sendimage(int len1, unsigned short * image_ptr, short unsigned int fcnum, short unsigned int arraynum, short unsigned int imagenum)
    {
       return py_network_sendimage(fcnum, arraynum, imagenum, image_ptr);
    }

%}

void py_network_send(char* data, int bytes_to_send);

void py_network_startserver(int portnum);
int py_network_sendfilename(char *filename);
int py_network_sendfcnum(int fcnum);
int py_network_waitforsend(void);
int py_network_waitforDMD(void);
void py_network_stopserver(void);
void py_network_iboot_on(void);
void py_network_iboot_off(void);
void py_network_shutdown(void);

