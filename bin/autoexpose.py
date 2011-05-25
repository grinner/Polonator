"""
================================================================================

Polonator G.007 Image Acquisition Software

Church Lab, Harvard Medical School
Written by Greg Porreca

autoexpose.py: automated routine to determine EM gain for each fluorescent 
channel

Release 1.0 -- 04-15-2008

This software may be modified and re-distributed, but this header must appear
at the top of the file.

================================================================================
"""

import sys
import os
sys.path.append(os.environ['POLONATOR_PATH']+'/polonator')  

import polonator.camera.asPhoenixFunctions as PC
from polonator.motion.maestro import MaestroFunctions
import logger
from polonator.tel_net import Tel_net
import random
import time

class Autoexpose:
    global camera
    global telnet
    global maestro
    global device_filters
    global logger
    
    def __init__(self):
        self.camera = PC
        self.maestro = Maestro_Functions()
        self.telnet = Tel_net()
        self.logger = LoggerFunctions.Logger_Functions('../logs/autoexpose-log')

    def autoe(self, flowcell, fluors, integration_times):
        # this maps a fluor name to an absolute position on the device filter wheel
        # the mapping is defined by initThetaIndex in AFMScanMotion which runs on
        # the controller
        # self.device_filters = {'fam': '1', 'cy3': '0', \
        #                        'cy5': '3', 'txred': '2', \
        #                        'white': '5', 'none': '4'}

        # these params specify how to step the gain
        gain_min = {'cy5': 60, 'cy3': 60, 'fam': 60, 'txred': 60, 'white': 10} 
        # starting (minimum) gains
        gain_incr = 10  # gain stepping increment
        mean_target = {'cy5': 3200, 'cy3': 3200, \
                        'fam': 8000, 'txred': 3200, \
                        'white': 6000} # pixel mean targets
        
        # this holds the gain setting determined by the algorithm for each color
        autoe_gains = {}

        # first, we need to determine which chambers we want to use;
        # we use one chamber from each column of 6 (3 total choices)
        chamber = []    # holds the indices of the chambers to visit
        chamberXC = []  # holds the X positions of the centers of the chambers
        chamberYC = []  # holds the Y positions of the centers of the chambers
        chamberX = []   # holds the X positions of all positions in all chambers
        chamberY = []   # holds the Y positions of all positions in all chambers
        chamberXAE = [] # X positions the autoexposure algorithm will visit 
                        # for all colors
        chamberYAE = [] # Y positions the autoexposure algorithm will visit 
                        # for all colors
        best_mean = []
        best_gain = []
        chamber_gain = []
        
        chamber.insert(0, random.randint(0,5))
        chamber.insert(1, random.randint(0,5))
        chamber.insert(2, random.randint(0,5))

        # now, we determine the actual X,Y positions of these chambers
        # we have the coords for the center of well 0 for each flowcell
        # we also have the spacing between well centers; these are from
        # the code running on the Maestro:
        X0CENTER = 3550000  # flowcell 0, array 0 center in X
        Y0CENTER = 28310000 # flowcell 0, array 0 center in Y
        X1CENTER = 3550000  # flowcell 1, array 0 center in X
        Y1CENTER = 12013276 # flowcell 1, array 0 center in Y
        WELL2WELL = int(204.91 * 1000 * 22) #pitch of 22mm (204.91 steps/um)
        EDGE_OFFSET = int(204.91 * 1000 * 6) #radius of 6mm

        for i in range(0,3):
            if(flowcell == 0):
                chamberXC.insert(i, X0CENTER + (chamber[i] * WELL2WELL));
                chamberYC.insert(i, Y0CENTER + (i * WELL2WELL * -1));
                self.logger.log('Autoexposure position %d, random chamber %d, '\
                                + 'center X: %d, Y: %d' % \
                                (i, chamber[i], chamberXC[i], chamberYC[i]))
            else:
                chamberXC.insert(i, X1CENTER + (chamber[i] * WELL2WELL));
                chamberYC.insert(i, Y1CENTER + (i * WELL2WELL * -1));
                self.logger.log('Autoexposure position %d, random chamber %d, '\
                                 + 'center X: %d, Y: %d' % \
                                (i, chamber[i], chamberXC[i], chamberYC[i]))

        """
        We want to be sure our algorithm samples the arrays at
        representative spots (assuming signal will be non-uniform).
        So, we start by looking at 3 points in each of 3 chambers.
        The 3 points are across the center of the chamber in X
        (X is the fast axis of the stage), so we do left edge, center,
        and right edge.

        For each chamber, we 'use' the position which gives the median
        metric value.  We therefore end up with a list of 3 points, one
        per chamber.  These 3 points are the ones used to determine the gains
        for all colors.  We determine the 'optimal' gain according to our
        metric, then take the median of these optima, and that becomes
        our gain for that color during the current cycle of acquisition.

        start by imaging all 9 positions in one color (Cy5)
        for each of the positions in chamberX/Y, we need to expand
        from the single, center, position to center, left and right
        of the array
        """
        # generate the list of positions to visit
        for i in range(0,3): #for each chamber
            for j in range(0,3): #for each position within the chamber
                chamberX.insert((i*3)+j, chamberXC[i])
                chamberY.insert((i*3)+j, (chamberYC[i] + ((j-1)*EDGE_OFFSET)))
                self.logger.log('Autoexposure position %d,%d, chamber %d, '+ \
                                'X: %d, Y: %d' % \
                                (i, j, chamber[i], chamberX[(i*3)+j], \
                                chamberY[(i*3)+j]) )

        # start with the first element of the arg list
        self.maestro.filter_goto(fluors[0])
        self.camera.py_set_exposure(integration_times[0])
        #pick the best position in each chamber
        for i in range(0,2): #for each chamber
            # reset mean and gain arrays
            best_gain = []
            best_mean = []

            for j in range(0,3): #for each position
                
                self.maestro.stage_goto(chamberX[(i*3) + j], \
                                        chamberY[(i*3) + j])

                if((i==0) and (j==0)):
                    self.maestro.autofocus_on()

                # step-up the gain until we pass the target mean, then record \
                # the 'target' gain in best_gain
                for k in range(0, 10): #for each gain setting from gain_min in 
                                       # increments of gain_incr
                    curr_gain = gain_min[fluors[0]] + (k * gain_incr)
                    self.camera.py_set_gain(curr_gain)
                    self.maestro.shutter_open()
                    curr_mean = self.camera.py_imagemean(self.camera.py_snapimage());
                    self.maestro.shutter_close()
                    self.logger.log('For position %d,%d, chamber %d, gain of '+ \
                                    '%d gave mean of %d' % \
                                    (i, j, chamber[i], int(curr_gain), \
                                    int(curr_mean) ) )
                    if(curr_mean > mean_target[fluors[0]]):
                        # we're past the target; use the last setting
                        self.logger.log('For position %d,%d, chamber %d, ' + \
                                        'best gain is %d with mean of %d' % \
                                        (i, j, chamber[i], int(curr_gain), \
                                        int(curr_mean)))
                        best_gain.insert(j, curr_gain - gain_incr)
                        best_mean.insert(j, curr_mean)
                        break
                    #handle the case where no mean hit the target
                    #this will eventually be where we step the integration time 
                    # and repeat
                    #for now, just return the highest gain
                    if(k == 9):
                        self.logger.log('AUTOEXPOSE ERROR: For position %d,'+\
                                        '%d, chamber %d, best gain is %d with ' + \
                                        'BAD mean of %d' % (i, j, chamber[i], \
                                        int(curr_gain), int(curr_mean)))
                        best_gain.insert(j, curr_gain)
                        best_mean.insert(j, curr_mean)
                    
            # take the median gain for the current chamber, and put the coords
            # of that position in the autoexposure arrays chamberAEE & chamberYAE
            best_position = best_mean.index(self.median_value(best_mean)) 
            # determine the index of the best for the chamber
            chamber_gain.insert(i, best_gain[best_position]) 
            # keep the best gain from each chamber
            chamberXAE.insert(i, chamberX[(i*3)+best_position]) 
            # keep the best position from each chamber
            chamberYAE.insert(i, chamberY[(i*3)+best_position])

        #assign the correct value for cy5 (the median of the 3 gains found)
        autoe_gains[fluors[0]] = self.median_value(chamber_gain)
        print 'For fluor %s, autoexposure algorithm found gain %d to be ' + \
              'best (score %d)' % \ 
              (fluors[0], autoe_gains[fluors[0]], \
                self.median_value(chamber_gain))
        
        # now iterate over all fluors, using the best positions from above
        # don't do the color we already did, though
        for fluor in fluors:
            
            if(fluor != fluors[0]): #we already did the cy5 autoexposure
                
                self.maestro.filter_goto(fluor)
                self.camera.py_set_exposure(integration_times[fluors.index(fluor)])
                best_gain = []
                best_mean = []

                for i in range(0, 2): #visit all 3 chambers
                    self.maestro.stage_goto(chamberXAE[i], chamberYAE[i])
                    for k in range(0, 10): # for each gain setting from gain_min 
                                           # in increments of gain_incr
                        curr_gain = gain_min[fluor] + (k * gain_incr)
                        self.camera.py_set_gain(curr_gain)
                        self.maestro.shutter_open()
                        curr_mean = self.camera.py_imagemean(self.camera.py_snapimage());
                        self.maestro.shutter_close()
                        self.logger.log('For fluor %s, chamber %d, gain of %d '+ \
                                        'gave mean of %d' % \
                                        (fluor, chamber[i], curr_gain, \
                                        curr_mean))
                        if(curr_mean > mean_target[fluor]):
                            # we're past the target; use the last setting
                            self.logger.log('For fluor %s, chamber %d, best' + \
                                            ' gain is %d with mean of %d' % \
                                            (fluor, chamber[i], curr_gain, \
                                            curr_mean) )
                            best_gain.insert(i, curr_gain - gain_incr)
                            best_mean.insert(i, curr_mean)
                            break
                        if(k == 9):
                            self.logger.log('AUTOEXPOSE ERROR: For position' + \
                                            ' %d,%d, chamber %d, best gain is' + \
                                            ' %d with BAD mean of %d' % \
                                            (i, j, chamber[i], int(curr_gain), \
                                            int(curr_mean)) )
                            best_gain.insert(i, curr_gain)
                            best_mean.insert(i, curr_mean)

                autoe_gains[fluor] = self.median_value(best_gain)
                self.logger.log('For fluor %s, autoexposure algorithm found ' + \
                                'gain %d to be best (score %d)' % \ 
                                (fluor, autoe_gains[fluor], \
                                best_mean[self.median_index(best_gain)]))
        #self.camera.__del__()
        return autoe_gains

    def median_index(self, x):  # x is a list of numbers; function will return 
                                # index of median
        if(len(x) == 0):
            self.logger.log('ERROR: cannot take the median of 0 values')
            return 0
        a = sorted(x)
        b = len(a)
        c = int(b/2) - 1
        d = x.index(a[c])
        return d

    def median_value(self, x): # x is a list of numbers; function will 
                               # return median
        if(len(x) == 0):
            self.logger.log('ERROR: cannot take the median of 0 values')
            return 0
        a = sorted(x)
        b = len(a)
        c = int(b/2) - 1
        return a[c]

