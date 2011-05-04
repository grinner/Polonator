"""
=============================================================================

Polonator G.007 Selective Illuminate Software

Wyss Institute
Written by Nick Conway

testPAcq.py: this script takes the the SWIGed version of 
Polonator-acquirer.c and tests the communications interface with the 
processing machine

Original version -- 2-2-2010 [NC]

    BEFORE running 
ssh to the processing machine 

ssh -X proc
password is "polonator" 
goto directory home/polonator/POLONATOR_DATE/DMD_test/
and run ./processor.pl
=============================================================================

"""

import sys
import getopt
import os
import time
import polonator.camera.asPhoenix as PC
from polonator.motion import maestro
from polProcDMD import *

DEFAULT_CUBE = "spare"
DEFAULT_TDI = 0

def main(argv=None):
    
    """
    
    Initial values of variables
    
    """
    ImageExposure = 0.001
    ImageGain = 20
    ImageColor = 'spare'
    ImageFilename = "/home/polonator/G.007/G.007_acquisition/DMD/image.raw"
    lane = 1
    image_num = 100
    flowcell = 0
    MF = maestro.MaestroFunctions()
    print "started Maestro\n"
    py_start_network_server()
    print "started server\n"
    # move to a particular image
    MF.go_to_image(flowcell,lane,image_num)
    print "moved to position\n"
    # take a picture
    PC.py_snap(ImageExposure,ImageGain, ImageColor, ImageFilename)
    print "snapped!!!\n"
    # send the picture to the processing machine
    py_send_DMD_register_image(lane, image_num)
    print "sent image\n"
    # grab the offsets
    x_offset = int(py_get_X_offset())
    y_offset = int(py_get_Y_offset())
    
       # take a picture
    PC.py_snap(ImageExposure,ImageGain, ImageColor, ImageFilename)
    print "snapped!!!\n"
    # send the picture to the processing machine
    py_send_DMD_register_image(lane, image_num)
    print "sent image\n"
    # grab the offsets
    x_offset = int(py_get_X_offset())
    y_offset = int(py_get_Y_offset())
    
    # print the output
    print "The X offset is %d again \n" % x_offset
    print "The Y offset is %d again \n" % y_offset
    
    time.sleep(2)
    py_stop_network_server()
    print "stopped server\n"
    """
       # Process command line
   
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
        testDMD.show_usage()
        sys.exit(-1)
    #end if
    
    MF = MaestroFunctions.Maestro_Functions()
    
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

    

    return 0
if __name__ == "__main__":
    sys.exit(main())
# end def
