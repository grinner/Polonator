"""
================================================================================

Polonator G.007 Selective Illuminate Software

Wyss Institute
Written by Nick Conway

testDMD_lib.py: Test script to determine if SWIG handles
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
 ===============================================================================
The Python Imaging Library (PIL) is

    Copyright © 1997-2006 by Secret Labs AB
    Copyright © 1995-2006 by Fredrik Lundh 
"""

import sys
import getopt
import os

import polonator.illum.D4000 as PI

import polonator.camera.asPhoenix as PC

import polonator.motion.maestro as maestro
from PIL import Image

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


def thing(expos=0.008, gain=2, mycube='spare', TDI_flag = 0):

    """
    Process command line
    """
    IlluminateWidth = 1920
    IlluminateHeight = 1080
    CameraWidth = 1000
    CameraHeight = 1000

    #MF = MaestroFunctions.Maestro_Functions()
    MF = maestro.MaestroFunctions()
    
    # process the numeric arguments
    int_time = float(expos)
    em_gain  = int(gain)


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
    if PI.py_illuminate_init(IlluminateWidth, IlluminateHeight, CameraWidth, CameraHeight) < 0:
        sys.exit(-1)
    #end if

    # load an identity map for alignment parameters
    # CRUCIAL as for now!!!!!!!!!!!!
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
    im = Image.open("alignment_DMD5.bmp")
    #im = Image.open("George_Church_03.bmp")
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
    #done = 0
    for y in range(0,IlluminateHeight,y_step):
        for x in range(0,IlluminateWidth,x_step):
            #if pix[y,x] != 0:
            if pix[y,x] == 0:# and done == 0:
                #x = 1920/2
                #y = 1080/2
                PI.py_illuminate_point(x,y,mask_number)
                #print x, y
                #done = 1;
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
    return 0
# end def

def expose():
    PI.py_illuminate_enable()
    PI.py_illuminate_expose()
#end def

def floaty():
    PI.py_illuminate_float()
#end def

def clear():
    PI.py_clear_framebuffer()
#end def

def off():
    PI.py_illuminate_disable()
#end def

def on():
    PI.py_light_all()
#end def

def snapPicPNG(exposure, gain, color, filename):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    temp = PC.snap(exposure, gain, color, filename + ".raw")
    im = Image.new('L', (1000,1000))
    pix = im.load()
    for i in range(1000):
        for j in range(1000):
            pix[j,i] = PC.py_14to8bit(temp, 1000*i+j)
        # end for
    # end for
    PC.cameraClose() # also frees up image buffer memory
    im.save(filename + ".png", "png")
# end def

def snapAllPics(exposure, gain):
    """
        exposure and gain are lists
        also creates an RGB overlay image
    """
    color0 = "cy3"
    color1 = "cy5"
    color2 = "txred"
    color3 = "fam"
    filename0 = "colorsnap-cy3"
    filename1 = "colorsnap-cy5"
    filename2 = "colorsnap-txred"
    filename3 = "colorsnap-fam"
    filename4 = "overlay-cy3-cy5-txRed"
    snapPicPNG(exposure[0], gain[0], color0, filename0)
    snapPicPNG(exposure[1], gain[1], color1, filename1)
    snapPicPNG(exposure[2], gain[2], color2, filename2)
    snapPicPNG(exposure[3], gain[3], color3, filename3)
    im = Image.new('RGB', (1000,1000))
    imR = Image.open(filename1 +".png") # cy5 is red
    imG = Image.open(filename0 +".png") # cy3 is green
    imB = Image.open(filename2 +".png") # txred is blue
    pix = im.load()
    pixR = imR.load()
    pixG = imG.load()
    pixB = imB.load()
    for i in range(1000):
        for j in range(1000):
            pix[i,j] = (pixR[i,j],pixG[i,j],pixB[i,j])
        # end for
    # end for
    im.save(filename4 +".png", "png")
# end def
