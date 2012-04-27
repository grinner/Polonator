"""
File to snap pictures with the andor camera and steve tubbs library
"""

CIW_PATH = "/home/nick/ciw/mar15"

import os
import sys
import subprocess
import time, glob
import Image
import numpy as np

def installdriver():
    os.system("gksudo insmod " + CIW_PATH + "/andor/bitflow/drv/bitflow.ko fwDelay1=800")

def start(exposure):
    # os.system("source setEnv")
    #os.system("/bin/bash -c 'cd " + CIW_PATH + "/ph2/ph1.1 ; source setEnv; ./doStartServer " + exposure + "'")
    subprocess.Popen(["/bin/bash", "-c", "cd " + CIW_PATH + "/ph2/ph1.1 ; source setEnv; ./doStartServer " + exposure])
# end def

def main(argv=None):
    if argv is None:
        argv = sys.argv
    # end if
    argc = len(argv)
    if argc < 2:
        installdriver()
    # end if
    if argv[1] == "start":
        start(argv[2]) if argc == 3 else start("")
    if argv[1] == "live":
        os.system("vlc v4l2:///dev/video1 &")
    if argv[1] == "close":
        subprocess.Popen(["/bin/bash", "-c", "cd " + CIW_PATH + "/ph2/ph1.1 ; source setEnv; bin/WFGItest -X"])
# end def

def snapLive(colorList=['cy3', 'cy5', 'txred', 'fam'], path=os.environ['HOME'], save=False):
    """
    List of colors to expose and exposure times
    """
    timestr = str(time.time()) if save else "s"
    for color in colorList:
        mf.filter_goto(color)
        time.sleep(1.1)
        fileout = timestr + "_" + color + ".bmp"
        fileout = path + "/" + fileout if path else fileout
        subprocess.Popen(["/bin/bash", "-c", CIW_PATH + "/ph2/tv4l2/tv4l2 -d -o 1 -i 3 -i 12 -B " + fileout + " -c"])
        time.sleep(1.5)
        subprocess.Popen(["/bin/bash", "-c", "eog " + fileout + " &"])
    # end for
# end def

def snap(colorList=['cy3', 'cy5', 'txred', 'fam'], path=os.environ['HOME'], save=False):
    """
    List of colors to expose and exposure times
    """
    timestr = str(time.time()) if save else "sr"
    files = []
    for color in colorList:
        mf.filter_goto(color)
        time.sleep(1.1)
        fileout = timestr + "_" + color
        subprocess.Popen(["/bin/bash", "-c",
                                    "cd " + CIW_PATH + "/ph2/ph1.1 ; source setEnv;"  + \
                                    "cd " + path + "; " + \
                                    #"rm " + fileout + "; " +  \
                                    CIW_PATH + "/ph2/ph1.1/bin/WFGIclientFile -d -t -p " + fileout])
        time.sleep(2.5)
        #subprocess.Popen(["/bin/bash", "-c", "eog " + fileout + " &"])
        files.append(onePNG(path+ "/" +fileout, autolevel=0))
    # end for
    time.sleep(1.5)
    execf = ' '.join(files)
    #subprocess.Popen(["/bin/bash", "-c", "eog " + execf + " &"])
    im_overlay = overlayPNG(files)
    subprocess.Popen(["/bin/bash", "-c", "eog " + im_overlay + " &"])
# end def

def onePNG(filename, autolevel=0):
    """
    snaps an image and saves the 16bit image as an 8 bit greyscale PNG
    by shifting out 8 bits
    returns the output filename if needed
    """
    shape = (2160, 2560+8)
    #image_file = open(filename+'.raw', 'rb')
    print "The file is ", filename
    filename = glob.glob(filename+"*.raw")[0]
    image_file = open(filename, 'rb')
    filename = os.path.splitext(filename)[0]
    # load a 1000000 length array
    img_array_1D = np.fromfile(file=image_file, dtype=np.uint16)
    print "the size of the image is", img_array_1D.shape, "should be ", 2568*2160
    image_file.close()
    img_array_2D = img_array_1D.reshape(shape)[:,8:]
    if autolevel == 1:
        auto_array = img_array_2D.astype(np.uint32)
        max_val = auto_array.max()
        min_val = auto_array.min()
        img_array_2D = (4095*(auto_array-min_val)/(max_val-min_val)).astype(np.uint16)
    # end if
    
    image_8bit = (img_array_2D >> 4).astype(np.uint8)
    im = Image.fromarray(image_8bit,'L')
    im.save(filename + ".png", "png")
    return filename + ".png"
# end def

def overlayPNG(files):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    """
    path = os.path.dirname(os.path.abspath(files[0]))
    pix_list = []
    for f in files:
        image_file = Image.open(f)
        pix_list.append(np.array(image_file).astype(np.uint16))
        #image_file.close()
    # end for
    # R = cy5, G = cy3, B = txr
    # yellow = fam ==> RGB:255,255,0
    rpix = pix_list[1].astype(np.uint8)#(pix_list[1] + pix_list[3]).clip(min=None,max=255).astype(np.uint8)
    #gpix = np.empty((2160,2560), dtype=np.uint8)
    gpix = pix_list[0].astype(np.uint8)#(pix_list[0] + pix_list[3]).clip(min=None,max=255).astype(np.uint8)
    bpix = pix_list[2].astype(np.uint8)
    print bpix.shape
    image_3D = np.dstack([rpix, \
                          gpix, \
                          bpix])
    image_8bit = image_3D #(image_3D >> 6).astype(np.uint8)
    im = Image.fromarray(image_8bit,'RGB')
    fileout = path + "/RGBY_overlay.png"
    im.save(fileout, "png")
    return fileout
# end def

def ops():
    mf.shutter_open()
def cls():
    mf.shutter_close() 

if __name__ == '__main__':
    main()
else:
    sys.path.append(os.environ['POLONATOR_PATH']+'/polonator')  
    from motion.maestro import MaestroFunctions as MF
    mf = MF()
