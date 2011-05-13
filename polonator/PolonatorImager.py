## ========================================================================================
##
## Polonator G.007 Image Acquisition Software
##
## Church Lab, Harvard Medical School
## Written by Greg Porreca
##
## PolonatorImager.py: main imaging thread executed during each sequencing cycle; performs
## either 'WL', or darkfield, imaging (averaging 1 or more images per position), or
## fluorescent imaging in 4 colors.  Sets variables on controller, performs autoexposure
## if necessary, then executes Polonator-acquirer.c, which collects the images from the
## camera and transmits them to the processing computer.
##
## Release 1.0 -- 04-15-2008
## Release 2.0 -- 01-27-2009 [GP] now handles TDI camera and new controller code
##
## This software may be modified and re-distributed, but this header must appear
## at the top of the file.
##
## ========================================================================================


import maestro
import threading
import time
import datetime
import os
import AutoExposeGain

class Imager(threading.Thread):
    global maestro
    global WL_num_to_avg
    global WL_integration_time
    global fluor_imaging
    global cycle_name
    global flowcell
    global dual_fc
    global integration_times
    global manual_gains
    global aexpose

    global num_arrays
    global num_imgs
    global TDI_flag
    global auto_exposure

    def __init__(self, cycle, fc):
        self.maestro = MaestroFunctions.MaestroFunctions()
        self.cycle_name = cycle
        self.flowcell = fc
        self.aexpose = AutoExposeGain.Autoexpose()

        self.TDI_flag = 0
        self.num_arrays = 1
        self.auto_exposure = 0;

        # is this being called to do darkfield imaging or fluorescence imaging?
        # darkfield imaging is specified by cycle name starting w/ 'WL';
        # assume all other cycle names specify fluorescence imaging
        # for darkfield imaging, numeric suffix on cycle name tells whether
        # the run is for one flowcell or two
        if(self.cycle_name.find('WL') is not -1):
            self.fluor_imaging = 0
            if(self.cycle_name.find('1') is not -1): # this is a single-fc run
                self.dual_fc = 0;
            else:
                self.dual_fc = 1; # WL2 specifies dual-flowcell run
                self.flowcell = self.flowcell + 2

            # specifies how many images to average for each WL image; we use 5 because
            # the time required to acquire & average 5 is approx equal to the time
            # required to segment the image
            self.WL_num_to_avg = 1

            # specifies the integration time (in msec) for brightfield imaging
            self.WL_integration_time = 250
            self.WL_gain = 70;

            # construct cycle name for WL imaging
            today = datetime.date.today()
            fn = '%02d%02d%04d' % (today.month, today.day, today.year)
            self.cycle_name = '%c%c%c%c%c%c' % (fn[0], fn[1], fn[2], fn[3], fn[6], fn[7])

        else:
            self.fluor_imaging = 1
            self.cycle_name = cycle;

        self.flowcell = fc
        # these are used for both manual exposures and autoexposure;
        # the order of these values corresponds to the fluor order
        # in array fluors[]
        self.integration_times = [35, 35, 35, 35, 35]
        self.manual_gains = [45, 120, 70, 50]
        #fluors = ['cy5', 'fam', 'cy3', 'txred']

        # this is run as a thread, so invoke Thread's constructor
        threading.Thread.__init__(self)



    def run(self):

        # are we doing fluorescent imaging (assume 4 colors) or
        # darkfield imaging?
        if(self.fluor_imaging == 1):
            if(0):
                self.aexpose = self.aexpose.autoe()
                self.manual_gains = self.aexpose
                print "using gains found with AutoExposGain.py"
            else:
                print "using gains entered by user"
            # this is the order of fluors in the integration and gain arrays;
            # it is also the order used by autoexposure
            fluors = ['cy5', 'fam', 'cy3', 'txred']

            # this is based on the nonamer synthesis, and is the sequence of the probe strand
            # therefore, the sequence of the template (bead-bound) strand is the complement
            fluor_to_base = {'cy5': 'T', 'cy3': 'A', 'txred': 'C', 'fam': 'G', 'none': 'N'}

            # order to do the fluorescence imaging; fam is the most photolabile, so we
            # do it first
            imaging_order = ['fam', 'cy3', 'txred', 'cy5', 'none']


            # now do 4 colors of imaging; each time, perform the stage alignment
            # first to minimize image alignment drift
            i = 0


            #print self.manual_gains
            #p = subprocess.Popen('/home/polonator/G.007/G.007_acquisition/PolonatorUtils snap ' + 0 + ' 0.035 ' + 90,  shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            #stdout_value, stderr_value = p.communicate()
            #print 'stdout_value \n' , stdout_value

            # cmd = '/home/polonator/G.007/G.007_acquisition/PolonatorUtils snap1 %d %d %d' % (0,0.035,90)
            # print cmd
            # os.system(cmd)

            while i < 4:

                # perform stage alignment
                cmd = (os.environ["POLONATOR_PATH"] + '/bin/Polonator-stagealign %d') % (self.flowcell)
                print cmd
                os.system(cmd)

                if(self.auto_exposure == 1):
                    j = 60
                    curr_name_new = '%s' % (imaging_order[i])
                    cmd = os.environ["POLONATOR_PATH"] + '/bin/PolonatorUtils gotostagealignpos 0 0'
                    print cmd
                    os.system(cmd)

                    cmd = os.environ["POLONATOR_PATH"] + '/bin/PolonatorUtils snap1 %s %f %d %d' % (curr_name_new,0.035,j, j+150)
                    print cmd
                    os.system(cmd)

                    # while j < 210:
                    #     cmd = '/home/polonator/G.007/G.007_acquisition/PolonatorUtils snap1 %s %f %d' % (curr_name_new,0.035,j)
                    #     print cmd
                    #     os.system(cmd)
                    #     j+=10


                # turn the darkfield illuminator off if it was left on
                self.maestro.darkfield_off()

                # setup controller for current color
                self.maestro.setup_imaging(imaging_order[i], self.integration_times[fluors.index(imaging_order[i])]-5, 1, self.num_arrays, self.flowcell, 0, self.TDI_flag)

                # start Polonator-acq which receives images from camera
                # and sends to processing pipeline
                #
                # construct current image-set name (cycle name plus base identity)
                curr_name = '%s_%s' % (self.cycle_name, fluor_to_base[imaging_order[i]])

                # this must be called w/ sudo so that the thread has a high enough priority
                # to ensure images aren't missed often
                cmd = ('sudo ' + os.environ["POLONATOR_PATH"] + '/bin/Polonator-acquirer %s %d %d %d %d') % (curr_name, self.integration_times[fluors.index(imaging_order[i])]-5, self.manual_gains[fluors.index(imaging_order[i])], self.flowcell, self.num_arrays)
                print cmd
                cmd = os.environ["POLONATOR_PATH"] + '/bin/Polonator-acquirer'
                print cmd

                # spawn thread to receive the images and wait for it to return;
                # it will exit w/ value of 10 if it did not acquire all images -- in
                # this case, repeat the current color
                if(os.spawnlp(os.P_WAIT, 'sudo', 'sudo', cmd,
                             curr_name,
                             str(self.integration_times[fluors.index(imaging_order[i])]),
                             str(self.manual_gains[fluors.index(imaging_order[i])]),
                             str(self.flowcell),
                             str(self.num_arrays)) != 10):
                    # move on to next color if all images were received
                    i+=1
                else:
                    print 'Polonator-acquirer exited with error'


        # we're doing darkfield imaging; assume we've just started the run and
        # tell Polonator-stagealign to zero out the offsetlog and collect a new
        # base image
        else:

            # generate the 'base' image for the stage alignment tool
            # if this is a dual-flowcell run, we must convert the flowcell == 2 or 3
            # to flowcell == 0 or 1
            if(self.dual_fc == 1):
                cmd = (os.environ["POLONATOR_PATH"] + '/bin/Polonator-stagealign %d 1') % (self.flowcell-2)
            else:
                cmd = (os.environ["POLONATOR_PATH"] + '/bin/Polonator-stagealign %d 1') % (self.flowcell)

            print cmd
            os.system(cmd)

            # we are doing darkfield imaging
            self.maestro.darkfield_on()

            self.maestro.setup_imaging('none', self.WL_integration_time-5, self.WL_num_to_avg, self.num_arrays, self.flowcell, 1, self.TDI_flag)

            # start Polonator-acq
            # during brightfield imaging (the start of the run), we need to tell the processing computer whether
            # it should expect 1 or 2 flowcells' worth of data; we do this by changing the flowcell number from
            # 0 for a single-flowcell run to either 2 or 3 for a dual flowcell run (fc 0 or 1, respectively)
            # the imaging software (Polonator-acquirer) understands this convention and converts the 2 or a 3
            # to a 0 or a 1 after sending the proper flag to the processing pipeline
            cmd = ('sudo '+ os.environ["POLONATOR_PATH"] + '/bin/Polonator-acquirer %s %d %d %d %d %d') % (self.cycle_name, self.WL_integration_time-5, self.WL_gain, self.flowcell, self.num_arrays, self.WL_num_to_avg)
            print cmd
            os.system(cmd)
            self.maestro.darkfield_off()

        return 1

