"""
===============================================================================

Polonator G.007 Selective Illuminate Software

Wyss Institute
Written by Nick Conway

testDMD.py: Test script to determine if SWIG handles 
for imaging, maestro, and DMD work

Ported to python from c and extended from Daniel Levner's
ReleaseFocus.c -- 1-9-2010 [Nick Conway]

This software may be modified and re-distributed, but this header must appear
at the top of the file.

1) move to spot with global offset
2) take image
3) send image to processing
4) receive from processing the offset and list of points
5) perform release on offset
    Need 0.06 220 for Fluorecense
    Need .001 30 + for general illumination 
python testDMD_bitmap.py 0.06 220 spare 0
================================================================================
"""

import sys
import getopt
import os
sys.path.append(os.environ['POLONATOR_PATH']+'/polonator')  

import polonator.illum.D4000 as PI
import polonator.camera.asPhoenix as PC
from polonator.motion.maestro import MaestroFunctions
# from PIL import Image
import Image # for later versions of PIL

DEFAULT_CUBE = "spare"
DEFAULT_TDI = 0

# show usage instructions
def show_usage():
    print "ReleaseFocus <int. time> <EM gain> [filter] [TDI_flag]\n"
    print "    int. time  -  integration time in seconds (floating point number)\n"
    print "    EM gain    -  gain for electron-multiplied camera\n"
    print "    filter     -  name of filter cube to be used (string)\n"
    print "    TDI_flag   -  0 = no TDI (default), 1-2 = TDI, defined by " + \
          "Polonator_live.c\n"
# end def


def main(argv=None):
    
    """
        Process command line
    """
    IlluminateWidth = 1920
    IlluminateHeight = 1080
    CameraWidth = 1000
    CameraHeight = 1000
    
    if argv is None:
        argv = sys.argv
    # end if
    argc = len(argv)
    print "newline\n"
    print "There are %d arguments\n" % argc     
    # make sure we have the correct number of arguments */
    if  (argc < 3) or (argc > 5):
        show_usage()
        sys.exit(-1)
    #end if
    
    MF = MaestroFunctions()
    
    # process the numeric arguments 
    int_time = float(argv[1])
    em_gain  = int(argv[2])

    # process cube name 
    if argc < 4:
        mycube = DEFAULT_CUBE
    # end if
    else:
        mycube = str(argv[3])
        
    # end else

    # process TDI flag
    if argc < 5:
        TDI_flag = DEFAULT_TDI
    # end if
    else:
        TDI_flag = int(argv[4])
    # end else

    """
        Set up imaging
    """

    MF.darkfield_off() 
    print "the third argument is %s\n" % mycube 
    MF.filter_goto(mycube)



    """
        set up release hardware
    """

    # initialize the release system 
    print "Initializing release system\n"
    PI.py_clear_memory();
    if PI.py_illuminate_init(IlluminateWidth, IlluminateHeight, \
                            CameraWidth, CameraHeight) < 0:
        sys.exit(-1)
    #end if
 
    # load an identity map for alignment parameters 
    PI.py_illuminate_alignment_load_identity()
   
    # enable release hardware 
    PI.py_illuminate_enable()

    # generate an image
    """
    im = Image.open("test.png")
  
    if im.size[0] > im.size[1]:
        scaler = IlluminateWidth/(im.size[0])
        im = im.resize((IlluminateWidth, scaler*im.size[1]),Image.BICUBIC)
    #end if
    else:
        scaler = IlluminateHeight/(im.size[1])
        im = im.resize((scaler*im.size[0], IlluminateHeight),Image.BICUBIC)
    #end else
    im.convert('L')
    im.show() 
    im.save("testout.png")
    """
    #im = Image.open("wyss_HD_R3.bmp")
    #im = Image.open("alignment_DMD.bmp")
    im = Image.open("George_Church_03.bmp")
    #im = Image.open("all_white.bmp")
    #im = im.rotate(90)
    im = im.transpose(Image.FLIP_LEFT_RIGHT)
    im = im.transpose(Image.FLIP_TOP_BOTTOM)
    #im = im.offset(50, -300)
    pix = im.load()
    
    mask_number = 0
    y_step = 1
    x_step = 1 
    #PI.py_clear_mask(mask_number)
    PI.py_clear_framebuffer()
    #PI.py_clear_memory()    # must run this or you will get line artifacts!!!!
  
    PI.py_illum_mask_radius_dmd(0,mask_number)
        
    for y in range(0,IlluminateHeight,y_step):
        for x in range(0,IlluminateWidth,x_step):
            #if pix[y,x] != 0:
            if pix[y,x] == 0:
                PI.py_illuminate_point(x,y,mask_number)
            #end if
        # end for
    # end for
 

    # expose image, so that it stays displayed 
    #PI.py_illuminate_disable()
    PI.py_illuminate_expose()

    """
    start imaging
    """

    # open shutter
    MF.shutter_open()

    # start imaging
    myargc = 3;
    myargv = ['0' ,'0','c','d']    # create a list of length 4
    myargv[2] = argv[1]
    myargv[3] = argv[2]

    # start in a separate process to avoid crash if window is closed
    
    #pid = os.fork()        # this is a UNIX only syscall
    #if pid == 0:
        #child process
     #   PC.py_camera_live(myargc, myargv, TDI_flag)
      #  return 0;
    # end if

    # in parent process 
    # wait for child to terminate 
    #os.waitpid(pid,os.WNOHANG)     # this option WNOHANG is UNIX only
    #os.wait()
    
    # close shutter
    #MF.shutter_close()
    return 0
# end def
if __name__ == "__main__":
    sys.exit(main())

