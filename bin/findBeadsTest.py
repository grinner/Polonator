#!/usr/bin/python
"""
findBeadsTest.py
5/5/2011
This is used to test the bead finding algorithms
The primary use of this script is to scan a flowcell and position by position:
1) "Before" find all objects of a particular color, save image
2) Release all objects of a particular color
3) "After" take a 2nd picture and overlay it on the first with the object 
    centroids to verify graphically release efficiency
4) Create multicolor overlays of the before and after images

This script is not intended to sequence. 
"""

import time
import sys
import os
sys.path.append(os.environ['POLONATOR_PATH']+'/polonator')  
  
import numpy as np
# from PIL import Image
import Image # for later versions of PIL
import polonator.image.findObjects as FO
from polonator.illum.mapping import MappingFunctions
from polonator.motion.maestro import MaestroFunctions
import polonator.camera.asPhoenix as PC
import polonator.image.imgTools as IT

# assumes we've alreay created a mapping file
MapFunc = MappingFunctions()
MapFunc.readMappingFile()

MaestroF = MaestroFunctions()

#my_color = "spare"
#my_color = "txred"
#my_color_ind = 2
my_color = "cy3"
my_color_ind = 0
no_color = "none"

color0 = "cy3"
color1 = "cy5"
color2 = "txred"
color3 = "fam"
filename0 = "colorsnap-cy3"
filename1 = "colorsnap-cy5"
filename2 = "colorsnap-txred"
filename3 = "colorsnap-fam"
filename4 = "overlay-cy3-cy5-txRed"    

filenames = [filename0, filename1,filename2,filename3]
colors = [color0,color1,color2,color3]
exposures = [.035, .035, .035,.035 ]
gains = [160,100,110,200]

LEN = 1000000
data_size = LEN#+4
MAX_BEADS_PERFRAME = 1000000
frames = 5

img_array = np.empty(data_size, dtype=np.uint16)
img_array_float = np.zeros(data_size, dtype=np.float)


img_array_flat = np.empty(data_size, dtype=np.uint16)
#bg_array = np.empty(data_size, dtype=np.uint16)
beadpos_xcol = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
beadpos_yrow = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
segmask_image =  np.empty(data_size, dtype=np.uint16)
num_beads = 0
    
def snapPicPNG(exposure, gain, color, filename):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    global img_array
    global img_array_float
    global frames
    
    global beadpos_xcol
    global beadpos_yrow
    global num_beads
    global my_color
    
    for i in range(frames): 
        PC.py_snapPtr(img_array, exposure,gain,color)
        img_array_float += img_array.astype(np.float)   # sum accumulator
   
    # end for
    
    img_array_out = (img_array_float/frames).astype(np.uint16) 
    
    img_array_float *= 0 # reset accumulator
    
    img_array_out.tofile(filename+'.raw')
    image_8bit = (img_array_out >> 6).astype(np.uint8)
    
    im = Image.fromarray(image_8bit.reshape((1000,1000)),'L')
    
    
    PC.cameraClose() # also frees up image buffer memory
    im.save(filename + ".png", "png")
    
    if num_beads != 0 and my_color == color:
        print("value at 1st point: %d" % \
               img_array_out[beadpos_xcol[0]+1000*beadpos_yrow[0]])
        print("value at 2nd point: %d" % \
               img_array_out[beadpos_xcol[1]+1000*beadpos_yrow[1]])
        print("value at 3rd point: %d" % \
                img_array_out[beadpos_xcol[2]+1000*beadpos_yrow[2]])
    # end if    
    del img_array_out
    del image_8bit
    del im
# end def


def snapAllPics(exposure, gain,suffix):
    """
    exposure and gain are lists
    also creates an RGB overlay image
    """
    global exposures
    global gains
    global MapFunc
    global MaestroF
    global colors
    global filenames

    MapFunc.illum_light_all()
    MaestroF.filter_goto(colors[0])
    snapPicPNG(exposures[0], gains[0], colors[0], filenames[0] + suffix)
    MaestroF.filter_goto(colors[1])
    snapPicPNG(exposures[1], gains[1], colors[1], filenames[1] + suffix)
    MaestroF.filter_goto(colors[2])
    snapPicPNG(exposures[2], gains[2], colors[2], filenames[2] + suffix)
    MaestroF.filter_goto(colors[3])
    snapPicPNG(exposures[3], gains[3], colors[3], filenames[3] + suffix)
    im = Image.new('RGB', (1000,1000))
    imR = Image.open(filenames[1] + suffix + ".png") # cy5 is red
    imG = Image.open(filenames[0] + suffix + ".png") # cy3 is green
    imB = Image.open(filenames[2] + suffix + ".png") # txred is blue
    pix = im.load()
    pixR = imR.load()
    pixG = imG.load()
    pixB = imB.load()
    for i in range(1000):
        for j in range(1000):
            pix[i,j] = (pixR[i,j],pixG[i,j],pixB[i,j])
        # end for
    # end for
    im.save(filename4 + suffix +".png", "png")
    PC.cameraClose() # also frees up image buffer memory
    del imR
    del imG
    del imB
    del im
    del pix
    del pixR
    del pixG
    del pixB
# end def

def release(mode=0,inverse=0,decimate=0,spot_size=0):
    """
    Releases the last found bead positions, or can illuminate a predetermined
    Set of bitmaps
    """
    global beadpos_xcol
    global beadpos_yrow
    global num_beads
    global MapFunc
    global MaestroF
    
    MaestroF.filter_goto("spare")
    if mode == 1:
        MapFunc.bitmap()
    elif mode == 2:
        MapFunc.george(inverse,decimate)
    elif mode == 3:
        MapFunc.wyss(inverse,decimate)
    else:
        if num_beads > 0 and num_beads < 20000:
            MapFunc.vector(beadpos_xcol,beadpos_yrow, num_beads,spot_size)
        # end if
    # end else
    MaestroF.shutter_open()
# end def

def stop_release():
    """
    Convenience command for command line
    Turns the DMD array entirely off by:
    1) floating the pixels
    2) clears the framebuffer
    3) closes the shutter
    """
    MapFunc.closeDMD()
    MaestroF.shutter_close()
# end def

def on():
    """
    Convenience command for command line
    Turns the DMD array entirely on pixel position
    """
    MapFunc.lightAll()
#end def

def find():
    """
    This function:
    1) Takes all images, saves with labels "_before" for all
    2) Then finds all the objects of one color "mycolor", 
        a) takes the images averaging over "frames" images
        a) finds all objects
        b) prints out a bunch of bead position values found in the image
    3) creates an overlay image putting the "mycolor" object centroids on top 
        of the raw images of the composite of all colors and saves a png
        of 'colorsnap-'+'find_overlay_' + my_color +'_before' + ".png
        
    """
    
    # declare all globals for function
    global img_array
    global img_array_float
    global img_array_flat

    global beadpos_xcol
    global beadpos_yrow
    global segmask_image
    
    global exposures
    global gains
    global colors
    global frames
    global num_beads
    global my_color
    global my_color_ind
    
    global MapFunc
    global MaestroF
    
    num_beads = 0
    MaestroF.darkfield_off()
    
    """
    DO NOT Home the filter wheel!!! In between sequencing and releasing 
    This destroys the mapping, since the filter whell cannot reapeatedly home
    To the same postion
    """
    #MaestroF.filter_home()
    
    snapAllPics(exposures, gains, "_before")
    
    MaestroF.filter_goto(my_color)
    time.sleep(1)
    
    for i in range(frames): 
        PC.py_snapPtr(img_array, \
                        exposures[my_color_ind], \
                        gains[my_color_ind] , \
                        my_color)
        img_array_float += img_array.astype(np.float)
    # end for
    
    img_array_out = (img_array_float/frames).astype(np.uint16) 
    img_array_float *= 0 # reset accumulator
    
    #invert for images where beads are lighter than background
    IT.flatten_image(1000, 1000, img_array_out, img_array_flat,1)
    img_array_flat = 16383 - img_array_flat # invert the 14 bit image
    num_beads = FO.find_objects(1000, 1000, \
                                img_array_flat, \
                                beadpos_xcol, beadpos_yrow, \
                                segmask_image)
    print("The number of beads found: %d" % num_beads)
    MapFunc.convertPicPNG2(16383-img_array_flat,0,1)
    MapFunc.convertPicPNG2(segmask_image*65535, 0,2)
    print("phase ONE complete!!!!!!!!!!!!!")
    print("value at 1st point: %d at point %d, %d" % \
           (img_array_out[beadpos_xcol[0]+1000*beadpos_yrow[0]], \
            beadpos_xcol[0], beadpos_yrow[0]))
    print("value at 2nd point: %d at point %d, %d" % \ 
            (img_array_out[beadpos_xcol[1]+1000*beadpos_yrow[1]], \
             beadpos_xcol[1], beadpos_yrow[1]))
    print("value at 3rd point: %d at point %d, %d" % \
            (img_array_out[beadpos_xcol[2]+1000*beadpos_yrow[2]], \
            beadpos_xcol[2], beadpos_yrow[2]))
    #PC.cameraClose() # also frees up image buffer memory
    stop_release()
    """
    imG = Image.open("map_0_1" +".png") # just beads
    imB = Image.open("map_0_2" +".png") # the found beads
    im = Image.new('RGB', (1000,1000))
    pix = im.load()
    pixG = imG.load()
    pixB = imB.load()

    for i in range(1000):
        for j in range(1000):
            pix[i,j] = (0,pixG[i,j],pixB[i,j]) #px = -238501 
        # end for
    # end for
    im.save("color_overlay_orig" +".png", "png")
    """
    
    """
    saves the pretaken raw 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    shape = (1000, 1000)
    img_1D_list = []
    for c in colors:
        img_file = open('colorsnap-'+c+'_before' +'.raw', 'rb')
        # load a 1000000 length array
        img_array_1D = np.fromfile(file=img_file, dtype=np.uint16)
        img_1D_list.append(img_array_1D.reshape(shape))
        img_file.close()
        del img_array_1D
    # end for
    # R = cy5, G = cy3, B = txRed
    # yellow = fam ==> RGB:255,255,0
    img_total = (img_1D_list[1] + \
                img_1D_list[2] + \
                img_1D_list[3]).clip(min=None,max=16383).astype(np.uint16)
    img_3D = np.dstack([img_total, \
                        16383*segmask_image.reshape(shape), \
                        img_1D_list[my_color_ind]])
    img_8bit = (img_3D >> 6).astype(np.uint8)
    
    #im = Image.fromarray(img_8bit.reshape(-1,1000*3),'RGB')
    im = Image.fromarray(img_8bit,'RGB')
    im.save('colorsnap-'+'find_overlay_' + my_color +'_before' + ".png", "png")
    
    # clean house
    del img_array_out
    del img_total
    del img_3D
    del img_8bit
    del im
    del shape
    del img_1D_list
# end def



def collect():
    """
    1) Snaps 4 pictures at the "exposures" and "gains" settings
    2) saves them to the "_after" image files
    3) closes the camera and the DMD
    4) creates an overlay png image for evaluation
    """
    global exposures
    global gains
    global my_color
    global my_color_ind
    
    #MapFunc.illum_light_all()
    snapAllPics(exposures, gains, "_after")

    PC.cameraClose() # also frees up image buffer memory
    stop_release()
    
    imR = Image.open("map_0_1" +".png") # just beads
    imG = Image.open("colorsnap-" + my_color + "_after" + ".png") # just beads
    imB = Image.open("map_0_2" +".png") # the found beads
    im = Image.new('RGB', (1000,1000))
    pix = im.load()
    pixR = imR.load()
    pixG = imG.load()
    pixB = imB.load()

    for i in range(1000):
        for j in range(1000):
            #pix[i,j] = (0,pixG[i,j],pixB[i,j]) #px = -238501 
            pix[i,j] = (pixR[i,j],pixG[i,j],pixB[i,j]) #px = -238501 
        # end for
    # end for
    im.save("color_overlay_current" +".png", "png")
    
    # free up the allocated memory
    del imR
    del imG
    del imB
    del im
    del pix
    del pixR
    del pixG
    del pixB
    
# end def

def overlayPNG(prefix,suffix):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    global colors
    shape = (1000, 1000)
    image_1D_list = []
    for c in colors:
        image_file = open(prefix+c+suffix+'.raw', 'rb')
        # load a 1000000 length array
        img_array_1D = np.fromfile(file=image_file, dtype=np.uint16)
        image_1D_list.append(img_array_1D.reshape(shape))
        image_file.close()
    # end for
    # R = cy5, G = cy3, B = txRed
    # yellow = fam ==> RGB:255,255,0
    image_3D = np.dstack([image_1D_list[1] + image_1D_list[3], \
                          image_1D_list[0]+image_1D_list[3], \
                          image_1D_list[2]])
    image_8bit = (image_3D >> 6).astype(np.uint8)
    
    #im = Image.fromarray(image_8bit.reshape(-1,1000*3),'RGB')
    im = Image.fromarray(image_8bit,'RGB')
    im.save(prefix+'_RGBY_overlay'+ suffix + ".png", "png")
# end def

def avgAndStd(filename, x=0,y=0):
    image_file = open(filename, 'rb')
    # load a 1000000 length array
    img_array_1D = np.fromfile(file=image_file, dtype=np.uint16)
    image_file.close()
    avg = np.average(img_array_1D)
    std_dev = np.std(img_array_1D)
    print avg, std_dev
    print("value is %d at point %d, %d" %  (img_array_1D[x+1000*y], x,y)) 
# end def

def snapUVPNG(exposure, gain,filename):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    global img_array
    global img_array_float
    global frames
    
    global beadpos_xcol
    global beadpos_yrow
    global num_beads
    
    for i in range(frames): 
        PC.py_snapPtr(img_array, exposure,gain,'spare')
        img_array_float += img_array.astype(np.float)   # sum accumulator
   
    # end for
    
    img_array_out = (img_array_float/frames).astype(np.uint16) 
    
    img_array_float *= 0 # reset accumulator
    
    img_array_out.tofile(filename+'.raw')
    image_8bit = (img_array_out >> 6).astype(np.uint8)
    
    im = Image.fromarray(image_8bit.reshape((1000,1000)),'L')
    
    
    PC.cameraClose() # also frees up image buffer memory
    im.save(filename + ".png", "png")
    
# end def

def overlay3(fileA, fileB):
    """
    overlays layers from a PNG file and a raw file
    """
    shape = (1000, 1000)
    image_fileA = Image.open(fileA +".png")
    image_fileB = open(fileB +'.raw', 'rb')
    # load a 1000000 length array
    pixB16 = (np.fromfile(file=image_fileB, dtype=np.uint16)).reshape(shape)
    auto_arrayB = pixB16.astype(np.uint32)
    max_val = auto_arrayB.max()
    min_val = auto_arrayB.min()
    pixB16  = (16383*(auto_arrayB-min_val)/(max_val-min_val)).astype(np.uint16)
    pixB = (pixB16 >> 6).astype(np.uint8)
    
    pixA = np.array(image_fileA)
    
    image_fileB.close()

    image_3D = np.dstack([pixB,pixA[:,:,1],pixA[:,:,2]])
    # end else
    
    im = Image.fromarray(image_3D,'RGB')
    im.save(fileA + "_and_" + fileB + ".png", "png")
# end def

def laneProcess(flowcell,lane,column):
    """
    process a "lane" 
    a "column" at a time at a "flowcell"
    Does all 218 images in a lane
    This can by used to release all beads that have mycolor
    """
    global MapFunc
    global MaestroF
    global num_beads

    minutes_to_wait = 1
    for i in range(218):
        print('processing number %d\n' % i)
        MaestroF.go_to_image(flowcell,lane,i+column*218)
        time.sleep(1)
        find()
        time.sleep(1)
        release(0,0,0,2)
        if num_beads > 10:
            time.sleep(minutes_to_wait*10)
        # end if
        else:
            time.sleep(1)
        # end else
        stop_release()
        time.sleep(1)
        #collect()
        #PC.cameraClose() # also frees up image buffer memory
    # end for
    print('done\n') 
# end def 
       

