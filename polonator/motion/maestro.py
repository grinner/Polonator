"""
================================================================================

Polonator G.007 Image Acquisition Software

Church Lab, Harvard Medical School
Written by Greg Porreca
modified by Nick Conway Wyss Institute

renamed to maestro.py by Nick Conway 09-20-2010
from MaestroFunctions.py: python maestro functionality; includes an interface to 
polonator_maestro.c (through SWIG- generated polonator_maestro.py)

Release 1.0 -- 04-15-2008
Release 2.0 -- 12-02-2008 Modified for PolonatorScan [GP]
Release 3.0 -- 09-29-2010 Modified for locking filter wheel and renamed [NC]

This software may be modified and re-distributed, but this header must appear
at the top of the file.

================================================================================
"""


import maestroFunctions
import tel_net
import time
import os


class MaestroFunctions:
    """
    class for 
    """
    global col  #controller object, for accessing C maestro functions
    global TS   #for python telnet session
    global device_filters
    global declared
    
    def torF(self):
        k = 0
        while True:
            if k < 3:
                k += 1
            # end if
            if k == 1:
                yield False
            # end if
            else:
                yield True
            # end else
        # end while 
       
    def __init__(self):
        if True:    # fix this with a static isDeclared variable
            MaestroFunctions.device_filters = {'fam': '1', 'cy3': '0', \
                                               'cy5': '3', 'txred': '2', \
                                                'spare': '5', 'none': '4'}
            MaestroFunctions.col = maestroFunctions   # ALIAS for the module
            MaestroFunctions.col.py_maestro_open('controller', 23)
            MaestroFunctions.TS = tel_net.Tel_net()
            MaestroFunctions.declared = True
        #end if


    """
    MOTION CONTROL FUNCTIONS
    """
    def setup_imaging(self,fluor, \ 
                            integration_time, \
                            num_imgs, num_lanes, \
                            fcnum, shutter_flag, \
                            TDI_flag):
        # this is done by C code through SWIG
        MaestroFunctions.TS.parse_read_string('x.iprdy=0', '>')
        MaestroFunctions.col.py_maestro_setupimaging( \
            int(MaestroFunctions.device_filters[fluor]), \
                integration_time, num_imgs, num_lanes, \ 
                fcnum, shutter_flag, \ 
                TDI_flag)
        self.stage_waitformotioncomplete()

    def go_to_image(self,flowcell,lane,image_number):
        MaestroFunctions.col.py_maestro_goto_image(flowcell, lane, image_number)
    
    def shutter_open(self):
        MaestroFunctions.col.py_maestro_shutter_open()

    def shutter_close(self):
        MaestroFunctions.col.py_maestro_shutter_close()

    def stage_goto(self,X, Y):
        # first, lock the stage
        self.stage_lock()

        # now, do the move
        write_string1 = 'y.pr='
        write_string1 += '%d' % (Y)
        write_string2 = 'x.pr='
        write_string2 += '%d' % (X)
        MaestroFunctions.TS.parse_read_string(write_string1, '>')
        MaestroFunctions.TS.parse_read_string(write_string2, '>')
        MaestroFunctions.TS.parse_read_string('y.bg', '>')
        MaestroFunctions.TS.parse_read_string('x.bg', '>')
        self.stage_waitformotioncomplete()

    def stage_waitformotioncomplete(self):
        a = MaestroFunctions.TS.parse_read_string('x.ms', '>')
        while(a[0] != '0'):
            a = MaestroFunctions.TS.parse_read_string('x.ms', '>')
        a = MaestroFunctions.TS.parse_read_string('y.ms', '>')
        while(a[0] != '0'):
            a = MaestroFunctions.TS.parse_read_string('y.ms', '>')

    def stage_lock(self):
        MaestroFunctions.col.py_maestro_lock()

    def stage_unlock(self):
        MaestroFunctions.col.py_maestro_unlock()

    def filter_goto(self, filtername):
        MaestroFunctions.col.py_maestro_setcolor(filtername)

    def filter_home(self):
        MaestroFunctions.col.py_maestro_hometheta()
    # end def
    
    def filter_lock(self):
        MaestroFunctions.col.py_maestro_locktheta()
    # end def

    def filter_unlock(self):
        MaestroFunctions.col.py_maestro_unlocktheta()
    # end def
    """
    ELECTRONIC CONTROL FUNCTIONS
    """    
    def darkfield_on(self):
        MaestroFunctions.col.py_maestro_darkfield_on()

    def darkfield_off(self):
        MaestroFunctions.col.py_maestro_darkfield_off()

    def cameraint_start(self):
        MaestroFunctions.TS.parse_read_string('x.ob[1]=1', '>')

    def cameraint_stop(self):
        MaestroFunctions.TS.parse_read_string('x.ob[1]=0', '>')
        
    def snap(self):
        MaestroFunctions.col.py_maestro_snap(int_time, shutterflag)
        
    def set_flag(self):
        MaestroFunctions.col.py_maestro_setflag()
        
    def reset_flag(self):
        MaestroFunctions.col.py_maestro_resetflag()
        
    def stop(self):
        MaestroFunctions.col.py_maestro_stop()
        
    def status(self):
        MaestroFunctions.col.py_maestro_getstatus()