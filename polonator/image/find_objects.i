%module findObjects
   
%{
    #define SWIG_FILE_WITH_INIT
    #include "find_objects.h"
%}
   
%include "numpy.i"
  
%init %{
    import_array();
%}

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * raw_image) }
%apply (int DIM1, unsigned short * INPLACE_ARRAY1) { (int len2, unsigned short * obj_xcol) }
%apply (int DIM1, unsigned short * INPLACE_ARRAY1)  { (int len3, unsigned short * obj_yrow) }
%apply (int DIM1, unsigned short * INPLACE_ARRAY1)  {(int len4, unsigned short * segmask_image) }

%include "find_objects.h"

%rename (find_objects) my_find_objects;

%inline %{
    unsigned short my_find_objects(int len_w, int len_h, \
        int len1, unsigned short * raw_image, \
        int len2, unsigned short * obj_xcol, \
        int len3, unsigned short * obj_yrow, \
        int len4, unsigned short * segmask_image) 
    {
        int temp = len_w*len_h;
        if (len1 != temp) 
        {
           PyErr_Format(PyExc_ValueError, "Array size error of lengths (%d,%d) given for images", len1, temp);     
        }
        if (len1 != len4) 
        {
            PyErr_Format(PyExc_ValueError, "Array mismatch of lengths (%d,%d) given for images", len1, len4);
        }
        if (len2 != len3) 
        {
            PyErr_Format(PyExc_ValueError, "Array mismatch of lengths (%d,%d) given for output points", len2, len3);
        }
        return find_objects(len_w, len_h, raw_image, obj_xcol, obj_yrow, segmask_image);
    }
%}
