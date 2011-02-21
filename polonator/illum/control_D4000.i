%module D4000


%{
    #define SWIG_FILE_WITH_INIT
    #include "illuminate_common.h"
    #include "control_D4000.h"
%}
   
%include "numpy.i"
  
%init %{
    import_array();
%}


%{
#include "illuminate_common.h"
#include "image_generator.h"
#include "control_D4000.h"
%}

%apply (int DIM1, int * IN_ARRAY1) { (int len1, int * vx) , (int len2, int * vy)}
%rename (py_illuminate_vector) my_py_illuminate_vector;


%inline %{
    int my_py_illuminate_vector(int len1, int *vx, int len2, int *vy, int mask_no, int num_points)
    {
       if (len1 != len2) 
       {
            PyErr_Format(PyExc_ValueError, "Arrays of lengths (%d,%d) given\n", len1, len2);
            return 0;
       }
       else
       {
            return py_illuminate_vector( vx,  vy, num_points, mask_no);
       }
    }
%}




%apply (int DIM1, int  * IN_ARRAY1) { (int len1, int * x) , (int len2, int * y)}
%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len3, unsigned short * image) }

%rename (py_illuminate_spot_find) my_py_illuminate_spot_find;

%inline %{
    int my_py_illuminate_spot_find(int len3, unsigned short *image, int len1, int * x, int len2, int * y)
    {
       if ((len1 != 1) || (len2 != 1))
       {
            PyErr_Format(PyExc_ValueError, "Arrays of lengths longer than 1 given: (%d,%d)\n", len1, len2);
            return 0;
       }
       else
       {
            return py_illuminate_spot_find(image,x, y);
       }
    }
%}  


void py_illuminate_point( int x, int y, int mask_num);
int py_illum_mask_radius(int rad, int mask_no);

void py_clear_framebuffer(void);
void py_clear_memory(void);
int py_illuminate_expose(void);
void py_clear_mask(int mask_no);
int py_illuminate_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight);

int py_illuminate_enable(void);
int py_illuminate_disable(void);
int py_light_all(void);
int py_illuminate_float(void);

int py_illuminate_alignment_load(float *al_params, int param_num);
int py_illuminate_alignment_load_identity(void);

