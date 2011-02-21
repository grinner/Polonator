%module imgTools
   
%{
    #define SWIG_FILE_WITH_INIT
    #include "img_tools.h"
%}
   
%include "numpy.i"
  
%init %{
    import_array();
%}


%apply (int DIM1, unsigned short * INPLACE_ARRAY1) { (int len1, unsigned short * orig_image) }
%apply (int DIM1, unsigned short * INPLACE_ARRAY1)  { (int len2, unsigned short * new_image) }

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len3, unsigned short * raw_image) }

%rename (flatten_image) my_flatten_image;

%inline %{
    void my_flatten_image(int len_w, int len_h,
                            int len1, unsigned short * orig_image, \
                            int len2, unsigned short * new_image, \
                            int rescale)
    {
        int temp = len_w*len_h;
        if (len1 != temp) 
        {
           PyErr_Format(PyExc_ValueError, "Array size error of lengths (%d,%d) given for images", len1, temp);     
        }
        if (len1 != len2) 
        {
            PyErr_Format(PyExc_ValueError, "Array mismatch of lengths (%d,%d) given for images", len1, len2);
        }
        flatten_image(len_w, len_h, orig_image, new_image, rescale);
    }
%}

%rename (stdev) my_stdev;

%inline %{
    int my_stdev(int len_w, int len_h,
                            int len3, unsigned short * raw_image)

    {
        int temp = len_w*len_h;
        if (len3 != temp) 
        {
           PyErr_Format(PyExc_ValueError, "Array size error of lengths (%d,%d) given for images", len3, temp);     
        }
        return stdev(len_w, len_h, raw_image);
    }
%}

%rename (stdev2) my_stdev2;

%inline %{
    int my_stdev2(int len_w, int len_h,
                            int len3, unsigned short * raw_image)

    {
        int temp = len_w*len_h;
        if (len3 != temp) 
        {
           PyErr_Format(PyExc_ValueError, "Array size error of lengths (%d,%d) given for images", len3, temp);     
        }
        return stdev2(len_w, len_h, raw_image);
    }
%}

%rename (mean) my_mean;

%inline %{
    int my_mean(int len_w, int len_h,
                            int len3, unsigned short * raw_image)

    {
        int temp = len_w*len_h;
        if (len3 != temp) 
        {
           PyErr_Format(PyExc_ValueError, "Array size error of lengths (%d,%d) given for images", len3, temp);     
        }
        return mean(len_w, len_h, raw_image);
    }
%}
 
