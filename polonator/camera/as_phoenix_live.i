%module asPhoenixLive
%{
    #define SWIG_FILE_WITH_INIT
    #include "as_phoenix_live.h"
%}
   
%include "numpy.i"
  
%init %{
    import_array();
%}


%include "as_phoenix_live.h"

%apply (int DIM1, unsigned short * IN_ARRAY1) { (int len1, unsigned short * frame_out) }


%rename (py_camera_live) my_py_camera_live;

%inline %{
    void my_py_camera_live(double exposure_time, int gain, int tdiflag, int len1, unsigned short *frame_out)
    { 
        py_camera_live(exposure_time, gain, tdiflag, frame_out);
    }
%}

int buffer_ready_count(void);
int buffer_overflow(void);



