%module asPhoenix

%{
    #define SWIG_FILE_WITH_INIT
    #include "as_phoenix.h"
%}
   
%include "numpy.i"
  
%init %{
    import_array();
%}

%include as_phoenix_functions.h
%include "as_phoenix.h"

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * raw_image) }


%rename (py_snapPtr) my_py_snapPtr;

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len2, unsigned short * the_image) }
%apply (int DIM1, unsigned char * IN_ARRAY1) { (int len3, unsigned char * image_copy) }

%rename (py_14to8bit) my_py_14to8bit;
%rename (py_14to8Image) my_py_14to8Image;

%inline %{
    void my_py_snapPtr(int len1, unsigned short * raw_image, float exposure, float gain, char * color)
    {
       py_snapPtr(raw_image, exposure, gain, color);
    }

%}

%inline %{
    unsigned char my_py_14to8bit(int len2, unsigned short * the_image, int ind)
    {
        return py_14to8bit(the_image, ind);
    }
%}

%inline %{
    void my_py_14to8Image(int len2, unsigned short * the_image, int len3, unsigned char *image_copy)
    {
        py_14to8Image(the_image, image_copy, len2);
    }
%}

unsigned short * snapPtr(float exposure, float gain, char *color);
int cameraLive(float exposure, int gain);
void cameraClose(void);

