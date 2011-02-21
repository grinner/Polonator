## ========================================================================================
##
## Polonator G.007 Image Acquisition Software
##
## Wyss Institute
## Written by Nick Conway
##
## testMap.py:
## 
##
## Release 1.0 -- 09-15-2010
## ========================================================================================
##

#import MappingFunctions
import polonator.illum.mapping as PM
   
MapFunc = PM.MappingFunctions()

def calibrate_camera_to_DMD_mapping():
    """
        this exists just to be able to generate the mapping from this module
    """
    MapFunc.map_gen()
# end def

def setupMap():
    """
        put the transform into memory
    """
    MapFunc.read_mapping_file()
#end def

def illum_point(x,y,check=0):
    MapFunc.illum_point_transform(x,y,check)
#end def

def show_George(transform=1):
    MapFunc.illum_bitmap(transform)
# end def
