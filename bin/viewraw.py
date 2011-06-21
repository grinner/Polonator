#!usr/bin/env python
# encoding: utf-8
"""
viewraw.py
Short code to convert a raw image to a png and then show it
"""
import os
import sys
sys.path.append(os.environ['POLONATOR_PATH']+'/polonator')  

import dataio.convert14to8 as convert

def show_usage():
    print "viewraw <filename>"
    print "filename - is the raw 14 bit image (16 bit per pixel) to convert"
# end def

def viewraw(filename):
    print "cooooooooool ", filename
    fileshow = convert.onePNG(filename, 1)
    # launch eye of gnome
    os.system("eog " + fileshow)
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
    viewraw(filename)
# end def

if __name__ == '__main__':
    main()

