#!usr/bin/env python
# encoding: utf-8
"""
convert14To8.py
Short code utilizing the python imaging library to convert a 14 bit raw image
Into a standard viewable PNG
"""
import numpy as np
import Image
import sys
import os

def show_usage():
    print "convert14to8 <filename> [autolevel]"
    print "filename - is the raw 14 bit image (16 bit per pixel) to convert"
    print "autolevel - optional, 1 autolevel, 0 no autolevel (default)"
    print "            whether to autolevel the image to zero out the"
    print "             \'dark pixels\'"
# end def

def onePNG(filename, autolevel=0):
    """
    snaps an image and saves the 14bit image as an 8 bit greyscale PNG
    by shifting out 6 bits
    returns the output filename if needed
    """
    shape = (1000, 1000)
    #image_file = open(filename+'.raw', 'rb')

    image_file = open(filename, 'rb')
    filename = os.path.splitext(filename)[0]
    # load a 1000000 length array
    img_array_1D = np.fromfile(file=image_file, dtype=np.uint16)
    if autolevel == 1:
        auto_array = img_array_1D.astype(np.uint32)
        max_val = auto_array.max()
        min_val = auto_array.min()
        img_array_1D = (16383*(auto_array-min_val)/(max_val-min_val)).astype(np.uint16)
    # end if

    image_file.close()
    image_8bit = (img_array_1D.reshape(shape) >> 6).astype(np.uint8)

    im = Image.fromarray(image_8bit,'L')
    im.save(filename + ".png", "png")
    return filename + ".png"
# end def

def main(argv=None):
    if argv is None:
        argv = sys.argv
    # end if
    argc = len(argv)
    if argc < 2:
        show_usage()
        sys.exit(-1)
    # end if
    filename = argv[1]
    if argc > 2:
        autolevel = argv[2]
    # end if
    else:
        autolevel = 0
    # end else
    onePNG(filename, autolevel)
# end def

if __name__ == '__main__':
    main()

