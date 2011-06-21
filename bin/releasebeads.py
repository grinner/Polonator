
"""
releasebeads.py
This file is used 

Polonator G.007 Image Acquisition Software

Wyss Institute
Written by Nick Conway

Release 1.0 -- 01-04-2010

List of important paths
polonator_data_dir = $HOME/polonator/G.007/acquisition
base_dir = polonator_data_dir/output_data 

This script releases all of the beads in the Basecall files (*.basecalls) in 
the 'base_dir' path, one basecall file at a time

also required are the object table file in the 'polonator_data_dir' and the 
hit summary file, '*.hit_summary', in the base_dir

"""

import os
import sys
sys.path.append( os.environ['POLONATOR_PATH'] + '/polonator' )  
import time
import glob

from polonator.illum.mapping import MappingFunctions
from polonator.motion.maestro import MaestroFunctions
import polonator.polProcDMD as PA # this is polonator acquirier
import polonator.camera.asPhoenix as PC
import polonator.logger as logger
import polonator.dataio.basecall as bc
import polonator.dataio.objectTable as ot
import numpy as np
from viewraw import viewraw
   
MaestroF = MaestroFunctions()
MapFunc = MappingFunctions()

beadpos_xcol = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
beadpos_yrow = np.empty(MAX_BEADS_PERFRAME, dtype=np.uint16)
num_beads = 0
EXPOSURE_TIME = 60 # in seconds, time to expose beads

def calibrate_camera_to_DMD_mapping():
    """
    Convenience function this exists just to be able to generate the 
    mapping from this module
    """
    # this will move the filter wheel to position and generate a mapping_file
    # after this, we DO NOT want to move the filter wheel at all!!!!
    MapFunc.mapGen()
    
    # show a bitmap to confirm to user that the calibration went appropriately
    MapFunc.george()
    MapFunc.snapImageAndSave()
    # show a picture of a snap for confirmation
    viewraw(MapFunc.snapImageFilename())
    
    # Request user interaction to confirm mapping.
    isOKtoBegin = raw_input("Press y to continue...")
    if isOKtoBegin == 'y':
        stop_release()
    # end if
    else:
        print("It appears the mapping didn't work out...")
        stop_release()
        sys.exit(1)
    # end else
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
    
def goto_flagged_images(self):
    """
    Function perform the following:
        
    1) open the basecall files and look for flagged beads
    2) open the object table and get the corresponding beads point
    3) If the image number read differs from the previous image number read 
       in the filestream goto the new image by commanding the Maestro 
       motorcontroller to do so
    4) Ditto (kindof) for flowcell and lane
    5) When you move to a new image perform a fine alignment:
        a) by taking a picture, and save file
        b) send file to processing machine
        c) wait for a response with the fine offset
    6) When you move to a new lane, do a coarse alignment with 
        "Polonator-stagealign"
    7) add the offset to the bead points from 2
    8) when you've read all the beads in an image according to the basecall 
       file, illuminate that image with a image mask on the DMD generated by 
       mapping to the DMD from the camera coordinates
    9) Continue for all images in a base call file
        
    The object table is parsed as follows since it is a binary file
    FIELD       VALUE                   SIZE       DESCRIPTION
    start       -1                      4 bytes    start of next block (1 
                                                   block per image)
    flowcell_id {0..FCs_PER_RUN - 1}    4 bytes    the number of flowcells    
                                                   currently on the Polonator 
                                                   (usually 1 or 2)
    array_id    {0..ARRAYS_PER_FC - 1}  4 bytes    the number of arrays the    
                                                   Polonator will image per 
                                                   flowcell (usually 18)
    image_id    {0..IMGS_PER_ARRAY - 1} 4 bytes    the number of images per 
                                                   array the Polonator will 
                                                   acquire (usually 2514)
    num_objects {0..65535}              4 bytes    maximum number of objects 
                                                   per frame is largest 
                                                   allowable short unsigned int
    beadpos_xcol{0..NUM_XCOLS - 1}      2 bytes    X image position of centroid
                                                   for current bead
    beadpos_yrow{0..NUM_YROWS - 1}      2 bytes    Y image position of centroid
                                                   for current bead

    The basecall file is parsed as follows since it is a tab delimited text file
    FLOWCELL#    LANE#      IMAGE#    BEAD#    X    X    X    FLAG
    one basecall file per lane
        
    the number of beads flagged per lane in a given basecall file 
    is in the *._ file
            
    """
    # now look up the base call file
    polonator_data_dir = os.environ['HOME'] + '/polonator/G.007/acquisition'
    acquire_exec_path = os.environ['POLONATOR_PATH'] + '/bin'
    base_dir = polonator_data_dir + 'output_data/'
    the_files = glob.glob(base_dir + '*.basecalls') # there is a basecall file 
                                                    # for each lane?
    the_object_table_file = polonator_data_dir + 'object_table'
    the_hit_summary_file = glob.glob(base_dir + '*.hit_summary')
    
    
    print the_files
        
    #in_file  = open(the_files[0], 'r')
    the_basecall = bc.BaseCall(the_files[0])
        
    #object_table = open(the_object_table_file,'rb')
    the_OT = ot.ObjectTable(the_object_table_file)
        
    last_image = -1     # outside the range of possible image numbers
    last_cell = -1
    last_lane = -1
    last_bead = 0
    num_beads = 0       # initialize to zero
    
    # start network communication for getting the image offset
    PA.py_start_network_server()
    
    coord_list = MapFunc.create_point_table(0xFFFF)
    
    MapFunc.load_transform_mapping()
    
    
    print('STATUS: begin reading basescall file')
    while True:
        the_basecall.read_entry()
        if (the_base_call.isEOF()):   # breaks the loop when a line length is 
                                      # zero indicating EOF
            #print 'breaking'
            #print 'a'+ base_call_line + 'b'
            print('STATUS: reached end of basescall file, line of zero length')
            break # if no longer reading a line
        else:    

            flowcell = the_basecall.get_flowcell() 
            lane = the_basecall.get_lane() 
            image_number = the_basecall.get_image_num() 
            bead = the_basecall.get_bead_num() 
            flag = the_basecall.get_flag() 
            
            if flag:
                # check to see if we don't need to move
                if last_image != image_number:
                    print('STATUS: new image in basecall file')
                    last_bead = 0
                    if num_beads > 0:
                        MapFunc.vector( beadpos_xcol, \
                                        beadpos_yrow, \
                                        num_beads, \ 
                                        spot_size=0)
                        time.sleep(EXPOSURE_TIME)
                        MapFunc.closeDMD()
                        MaestroF.shutter_close()
                        num_beads = 0   # reset the bead number
                    # end if
                    test_case = 0   # initialize variable so we can loop to 
                                    # find an image header in the object table
                    while (test_case != -1):  # burn through object table to 
                                              # match base call   
                        raw = theOT.read(4)
                        if len(raw) == 0:     # if the read returned zero bytes 
                                              # we reached EOF
                            print "Error: ReleaseBeads.py:' \
                                    + ' reached end of file too early likely!"
                            break
                        #end if
                        else :
                            test_case = (struct.unpack('i', raw))[0]
                        #end else
                    #end while
                    
                    #########################
                    # This section synchronizes base call location with object 
                    # table location
                  
                    last_bead = 0   # reset last_bead counter b/c we don't care 
                                    # once we are out of the first seek
                    # Match Flow_cell
                    if the_OT.get_flowcell() < flowcell:
                         the_OT.skip_to_next_flowcell()
                         the_OT.read_entry() # important to read in the entry 
                                             # of interest!

                    elif the_OT.get_flowcell() > flowcell:
                        print('Error: somehow flow cell order mismatch' \
                                + ' between base call and object table')
                    else:
                        pass
                    # end else
                    
                    # match lane
                    if the_OT.get_lane() < lane:
                        the_OT.skip_lane_N(lane) # skip to the "lane"th lane
                        the_OT.read_entry()      # important to read in the 
                                                 # entry of interest!
                    # end if
                    elif the_OT.get_lane() > lane:
                        print('Error: somehow passing basecall files out' \
                               +' of order')
                    # end elif
                    else:
                        pass
                    # end else

                    # match image number
                    if the_OT.get_image_num() < image_number:
                        the_OT.seek_image(image_number)
                        the_OT.read_entry()    # important to read in the entry 
                                               # of interest!
                    # end if
                    elif the_OT.get_image_num() > image_number:
                         print('Error: somehow basecall file' \
                                + 'has images out of order with object table')
                    # end elif
                    else:
                        pass
                    # end else
    
                    # now do the move
                    if last_cell != flowcell:   # move to new flowcell and 
                                                # align if necessary
                        cmd = acquire_exec_path
                        cmd = cmd + ('Polonator-stagealign %d 1' % (flowcell) )
                        print cmd
                        os.system(cmd)
                        last_cell = flowcell                            
                    #end if
                    MaestroF.goto_image(flowcell,lane,image_number)  # move to new 
                                                               # image
                    last_image = image_number
                    print 'Moved to image number %d ' % image_number
                    time.sleep(0.1)
                    
                    # call image grabber
                    MapFunc.snap_image_and_save()
                    
                    # let proc do alignment
                    PA.py_send_DMD_register_image(lane,image_number)
                    
                    # get the sub_alignment of pixels these are ints
                    x_offset = int(PA.py_get_X_offset())
                    y_offset = int(PA.py_get_Y_offset())
                # end if    
                # read_through object table to match to bead in the base call
                # assumes this is within a single image
                the_OT.seek_bead_num(bead)
                last_bead = bead   # make sure we are in the right spot in the  
                                   # object table
                # end if
                print 'reading object table'
                the_OT.read_point() # load point data into memory and advance 
                                    # pointer
                x_point = the_OT.get_x() + x_offset
                y_point = the_OT.get_y() + y_offset
                if (x_point < 0) or (x_point > MapFunc.CameraWidth):
                    print 'ERROR: ReleaseBeads x point out of bounds'
                    pass    # don't do anything
                # end if
                elif (y_point < 0) or (y_point > MapFunc.CameraWidth):
                    print 'ERROR: ReleaseBeads: y point out of bounds'
                    pass    # don't do anything 
                # end elif
                else: 
                    print 'STATUS: Release Beads: adding point to list'
                    # MapFunc.add_point_to_table(x_point, \ 
                    #                            y_point, \
                    #                            num_beads, \
                    #                           coord_list)
                    beadpos_xcol[num_beads] = x_point
                    beadpos_yrow[num_beads] = y_point
                    bead_last = bead
                    num_beads += 1  
                # end else
            #end if
        #end else       
    # end while True loop
    if num_beads > 0:
        # this function does the coordinate transform and illuminates!!!!
        # MapFunc.transform_camera_to_DMD( coord_list, num_beads - 1)
        MapFunc.vector(beadpos_xcol,beadpos_yrow, num_beads,spot_size=0)
        time.sleep(EXPOSURE_TIME)
        MapFunc.closeDMD()
        MaestroF.shutter_close()
        num_beads = 0 
    # end if
    #MapFunc.delete_point_table(coord_list) 
    the_basecall.close()
    the_OT.close()
    PA.py_stop_network_server()
#end def

def main():
    """
    Calibrate mapping, confirm mapping, then release
    """
    calibrate_camera_to_DMD_mapping()
    
    goto_flagged_images()
# end def

if __name__ == '__main__':
    main()
