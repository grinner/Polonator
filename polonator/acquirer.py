# The MIT License
#
# Copyright (c) 2011 Wyss Institute at Harvard University
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# http://www.opensource.org/licenses/mit-license.php

import polonator.illum.mapping as PM
from polonator.motion import maestro
import polonator.camera.asPhoenix as PC
import polonator.logger as PL
import polonator.networkG007 as net
import sys
import os
import numpy
import ConfigParser
import time

config = ConfigParser.ConfigParser()

polpath = os.environ["POLONATOR_PATH"]
acqcfg = polpath + "/config_files/acquisition.cfg");
config.read(acqcfg)

LEN = 1000000
data_size = LEN#+4

function_name = "acquirer"

# image averaging
# holds the final averaged image
average_image = numpy.zeros(data_size, dtype=np.uint16)
# holds the accumulating sums for each pixel
average_image_sums = numpy.zeros(data_size, dtype=np.uint32)

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

    mf = maestro.MaestroFunctions()
    img_buffer_cpy = numpy.empty(data_size, dtype=np.uint16)

    TOTAL_FCS = 1
    first_average_image = 1

    auto_exposure = 0
    
    base_dir = "%s/polonator/G.007/acquisition" % (os.environ["HOME"])
    log_dir =  "%s/logs" % (base_dir)
    command_buffer =  "mkdir -p %s" % log_dir
    system(command_buffer);

    logfilename = log_dir + "/" + config.get("Log", "logfilename") +\
                    "." + argv[1] + ".log"
    PL.start_logger(logfilename, 1)
    
    TOTAL_IMGS = int(config.get("Flowcell", "imgs_per_array"))

    image_timeout = int(config.get("Motion", "image_timeout"))

    PHX_buffersize = int(config.get("Camera", "PHX_buffersize"))

    proc_portnum = int(config.get("Processor", "proc_portnum"))

    PHX_configfilename  = polpath + "/config_files/" + \
                                config.get("Camera", "PHX_configfilename")

    auto_exposure = int(config.get("Stage_Align", "auto_exposure"))

    argc = len(argv)
    if argc == 7 :
        # number of images to average was specified 
        log_string = "%s called with %d args: <%s> <%d> <%d> <%d> <%d> <%d>", %
                        (argv[0], argc-1, argv[1], \
                        int(argv[2]), int(argv[3]), int(argv[4]),\
                        int(argv[5]), int(argv[6]) )
        PL.p_log(log_string)
        # num_to_avg consecutive images should be averaged together, 
        # then transmitted
        num_to_average = int(argv[6]) 
        if num_to_average == 1:
            num_to_average = 0
    elif argc == 6:
        # number of images to average was not specified; assume no averaging 
        log_string = "%s called with %d args: <%s> <%d> <%d> <%d> <%d>", % \
                        (argv[0], argc-1, argv[1], \
                        int(argv[2]), int(argv[3]), \
                        int(argv[4]), int(argv[5]) )
        PL.p_log(log_string)
        num_to_average = 0 # don't do image averaging
    else:
        print "ERROR: %s called with %d args, must be called as\n" % (argv[0], argc-1)
        print "       %s <cycle_name> <integration time in seconds> <EM gain> <flowcell number> <number of arrays>        <num_imgs_to_average>  OR\n" % (argv[0])
        print "       %s <cycle_name> <integration time in seconds> <EM gain> <flowcell number> <number of arrays> IF no image averaging desired.\n" % (argv[0])
        sys.exit(1)
    fcnum = int(argv[4])
    TOTAL_ARRAYS = int(argv[5])

    # CAMERA SETUP
    # configure framegrabber
    exposure = float(argv[2])
    gain = int(arg[3])
    PC.py_cameraInitAcq(0, exposure, gain)
    
    """
    NETWORK TRANSFER SETUP
    open the port, and wait for processor to connect
    """

    if auto_exposure == 1:
        net.py_network_startserver(proc_portnum)

        array_string = argv[1]

        p_log(array_string);

        if argv[1] == "0" or argv[1] == "1":
            p_log(array_string);
            send_initial_raw_images(1)
        else:
            log_string = "STATUS:\tPolonator-acquirer: running autoexposure, server sending filename %s to processor, port %d..." % (argv[1], proc_portnum)
            p_log_simple(log_string)


            # now send the filename for the current image set
            log_string = "STATUS:\tPolonator-acquirer: server sending filename %s to processor, port %d..." % (argv[1], proc_portnum)
            PL.p_log(log_string)
            net.py_network_sendfilename(argv[1])

            # close connection, then re-open for flowcell number
            net.py_network_stopserver()
            net.py_network_startserver(proc_portnum);

            autoe_gain = send_FL_images(array_string,15)
            PC.py_set_gain(autoe_gain)

            log_string = "STATUS:\tPolonator-acquirer: using the auto-exposure gain %d..." % (autoe_gain)
            p_log_simple(log_string)
        # end else

        net.py_network_stopserver()
    # end if



    p_log_simple("STATUS:\tPolonator-acquirer: server waiting for processor to connect...");
    net.py_network_startserver(proc_portnum)

    # now send the filename for the current image set
    log_string = "STATUS:\tPolonator-acquirer: server sending filename %s to processor, port %d..." % (argv[1], proc_portnum)
    PL.p_log(log_string)
    net.py_network_sendfilename(argv[1])

    # close connection, then re-open for flowcell number
    net.py_network_stopserver()
    net.py_network_startserver(proc_portnum)
    log_string = "STATUS:\tPolonator-acquirer: server sending flowcell %d to processor, port %d..." % (fcnum, proc_portnum)
    PL.p_log(log_string)
    net.py_network_sendfcnum(fcnum)

    # close connection, then re-open for image xfer
    net.py_network_stopserver()

    log_string = "STATUS:\tPolonator-acquirer: server establishing image transfer connection to processor, port %d..." % (proc_portnum)
    p_log_simple(log_string);
    net.py_network_startserver(proc_portnum)

    log_string = "STATUS:\tPolonator-acquirer: server establishing image transfer connection to processor, port %d..." % (proc_portnum)
    PL.p_log(log_string);

    blank_image[:] = 0
    
    p_log_simple("STATUS:\tPolonator-acquirer: waiting for processor to signal ready to begin");
    if net.py_network_waitforsend() != 1:
        p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
        exit_on_error(mf)
    p_log_simple("STATUS:\tPolonator-acquirer: processor ready to receive first image");
    net.py_network_sendimage(0, 0, 0, blank_image);


    # MAESTRO SETUP
    p_log("STATUS:\tPolonator-acquirer: opening connection to Maestro...");
    

    # IMAGE CAPTURE AND TRANSMIT
    # start capture on framegrabber
    PC.py_startAcquire()

    # initialize counters for image information transmitted to processor
    curr_fc = 0
    curr_array = 0
    curr_img = 0

    # initialize image averaging vars
    curr_averageimgnum = 0 
    averaging_complete = 0 


    sleep(5) 

    caught_readout = 0 

    # added
    mf.filter_goto("cy5")
    mf.snap(60 * 1000.0, 1)
    while not py_snapReceived():
        pass
    mf.filter_goto("cy3")
    mf.snap(60 * 1000.0, 1)
    while not py_snapReceived():
        pass
    mf.filter_goto("txred")
    mf.snap(60 * 1000.0, 1)
    while not py_snapReceived():
        pass

    # capture all images for the current imaging cycle
    # we're at the end once curr_fc has been incremented
    tv2 = time.gmtime()
    PC.sPCI_set_readout(1)

    while curr_fc < 1:

        tv1 = time.gmtime()
        wait_time = tv1 - tv2

        """
        if we're imaging two flowcells, during brightfield imaging fcnum will be
        2 or 3 for 1 or 2 flowcells, respectively; now that we've told the
        processing computer it should expect two flowcells, change the value so it
        reflects flowcell number 0 or 1 when inserted into each image header
        """
        if fcnum > 1:
            fcnum = fcnum - 2
        # end if

        """
        the acquisition cycle is as follows:
        -wait for image readout from previous acquisition to begin
        -wait for processor to be ready to handle image being read out
        -set iprdy flag on controller so imaging continues at next position
        -wait for readout to framegrabber to complete (image_ready interrupt)
        -send image to processor
        -loop
        """

        # callback hit; readout has begun on the current image
        if PC.sPCI_readout_started():
            caught_readout = 1
            log_string =  "STATUS:\tPolonator-acquirer: main loop evaluated readout_started==true for image %d" % (sPCI.num_imgs)
            PL.p_log(log_string)
            PC.sPCI_set_readout(0)
            if PC.sPCI_num_imgs() and num_to_average == 0:
                p_log("STATUS:\tPolonator-acquirer: wait for processor ready to receive image")
                if net.py_network_waitforsend() != 1 :
                    p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)")
                    exit_on_error(mf)
                # end if
            # end if
            p_log("STATUS:\tPolonator-acquirer: processor ready to receive image")
            # set controller flag so acquisition continues
            p_log("STATUS:\tPolonator-acquirer: signalling controller ready to start integration of next image");
            mf.setflag()
            p_log("STATUS:\tPolonator-acquirer: finished signalling controller ready to start integration of next image")

        # callback hit; a new image is ready to be handled
        if PC.sPCI.image_ready():
            # record current time so we know how long until the next image
            tv2 = time.gmtime();

            # get pointer to image so we can manipulate it */
            PC.py_startAcquire()
            
            """
            if averaging is turned on, do the averaging, then transmit if averaging is complete; */
            if averaging is off, transmit the image
            """
            if num_to_average:
                # add current buffer to the average 
                p_log("add current image to average")
                average_images(PC.py_get_buffer_ptr(img_buffer_cpy))

                if averaging_complete:
                    if not first_average_image:
                        p_log("STATUS:\tPolonator-acquirer: averaging complete; wait for processor ready to receive image");
                        while net.py_network_waitforsend() !=1:
                            p_log("ERROR:\tPolonator-acquirer: processor requested the wrong kind of data (not an image)");
                            exit_on_error(mf)
                    # end if
                # end if
                else:
                    first_average_image = 0;
                # end else
                    p_log("STATUS:\tPolonator-acquirer: processor ready to receive image")

                    # transmit the image; acquisition of next image has already started */
                    p_log("STATUS:\tPolonator-acquirer: sending image to processor")
                    net.py_network_sendimage(fcnum, curr_array, curr_img, average_image);
                    p_log("STATUS:\tPolonator-acquirer: finished sending image to processor")
            # end if

            # end if (num_to_average)
            else:
                # transmit the image; acquisition of next image has already started
                p_log("STATUS:\tPolonator-acquirer: sending image to processor");
                net.py_network_sendimage(fcnum, curr_array, curr_img, PC.py_get_buffer_ptr(img_buffer_cpy))
                p_log("STATUS:\tPolonator-acquirer: finished sending image to processor")
            # end else

            # release buffer back to Phoenix's buffer pool 
            PC.py_release_buffer()

            # reset flag so callback knows the image is finished being handled
            p_log("STATUS:\tPolonator-acquirer: reset callback flag")
            PC.sPCI_set_image_ready(0)

            # changed on 100609
            mf.set_flag()

            # update image, array, fc counters
            if not num_to_average or  averaging_complete:
                log_string = "Finished handling\t%d\t%d\t%d\t%d" % \
                    (fcnum, curr_array, curr_img, sPCI.num_imgs)
                p_log_simple(log_string)
                curr_img += 1
                if curr_img == TOTAL_IMGS: # ready to increment array counter, reset image counter */
                    curr_img = 0
                    curr_array += 1
                    print "Finished handling\t  \t  \t    \t            "
                    if curr_array == TOTAL_ARRAYS:
                        # ready to increment flowcell counter, reset array counter */
                        curr_array = 0
                        curr_fc += 1
                        print  "Finished handling\t  \t  \t    \t          "
                    # end if
                # end if
            # end if
            p_log("STATUS:\tPolonator-acquirer: re-entering image-receive loop...");
        # end if (sPCI.image_ready)
    # end while
    print "\n"
    p_log("STATUS:\tPolonator-acquirer: last image received from camera...");
    mf.reset_flag()

    p_log("STATUS:\tPolonator-acquirer: release camera handle...");
    PC.py_cameraClose()
        
    net.py_network_waitforsend()  # don't close connection until last image is transmitted
    net.py_network_stopserver()
    mf.stop()
    return 0
# end def



def exit_on_error(maestro):
    p_log_errorno("ERROR: we've had either a camera or network error; exiting prematurely and signalling PolonatorImager to retry the current scan")
    maestro.status()
    PC.py_cameraClose()
    net.py_network_stopserver()
    maestro.stop()
    p_log_errorno("ERROR: exiting")
    sys.exit(10)
# end def



# used to accumulate an average of multiple images; we call this during */
# brightfield image acquisition to decrease noise so segmentation works better */
def average_images(curr_img):
    """
    this is a global variable that tells us how many images 
    will have been averaged after the current call completes 
    calling function uses this to determine whether it's time
    to transmit the image
    """
    global datasize
    global average_image
    global average_image_sums
    
    curr_averageimgnum += 1
    
    if curr_averageimgnum == num_to_average:
        # this image is the last one; add it, then compute avg
        for i in range(datasize):
            average_image_sums[i] += curr_img[i]
            average_image[i] = average_image_sums[i] / num_to_average
        # end for
        curr_averageimgnum = 0 # reset for next position */
        averaging_complete = 1 # signal to transmit the data in average_image */
    # end if
    elif curr_averageimgnum == 1:
        # first time for this position; overwrite previous data */
        averaging_complete = 0 # reset so nothing is transmitted until we do num_to_avg images */
        average_image_sums = numpy.copy(curr_img)
    else:
        for i in range(datasize):
            average_image_sums[i] += curr_img[i]
        # end for
    # end else
# end def


def send_initial_raw_images(num_array):
    #Network transfer declarations
    # int serv_sock; # socket for the server (this machine)
    i = 0
    j = 0
    global datasize

    # prepare the network connection and send
    # open the port, and wait for processor to connect
    net.py_network_startserver(proc_portnum)

    while i < num_array:
        # read and send the auto_exposure raw images
        stagealign_rawimgfilename = "%s/polonator/G.007/acquisition/stagealign/stagealign-image0_%d.raw" % (os.environ["HOME"], i)
        p_log_simple(stagealign_rawimgfilename);

        log_string =  "STATUS:\t 1try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string)

        baseimage = numpy.fromfile(file=stagealign_rawimgfilename, dtype=np.uint16)
        
        log_string = "STATUS:\t 2try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string)
 
        blank_image = numpy.copy(baseimage)
        j = 0
        log_string =  "STATUS:\t 3try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string)

        while net.py_network_waitforsend() != 1:
            log_string = "Waiting to send the autoexposure images"
            PL.p_log(log_string);
        # end while
        log_string = "Trying to send autoexposure images %d" %  (i)
        p_log_simple(log_string);
        network_sendimage(clnt_sock, 0, 0, i, blank_image);
        log_string = "Sent autoexposure images %d" % (i)
        p_log_simple(log_string);

        i += 1
    # end while
# end def

def send_FL_images(mystring, num_array):
    # prepare the network connection and send
    # open the port, and wait for processor to connect
    net.py_network_startserver(proc_portnum)

    string1 = mystring + len(mystring) - 1
    PL.p_log(string1)

    log_string = "string1 length %d" % (len(string1))
    PL.p_log(log_string)

    if string1 == "G":
        log_string = "success_%d.raw" % (0)
        PL.p_log(log_string);

        stagealign_dir_name = "%s/polonator/G.007/acquisition/autoexp_FL_images/fam/" % (os.environ["HOME"])
        PL.p_log(stagealign_dir_name)
    elif string1 == "A":
        log_string = "success_%d.raw" % (0)
        PL.p_log(log_string)

        sprintf(stagealign_dir_name, "%s/polonator/G.007/acquisition/autoexp_FL_images/cy3/" % (os.environ["HOME"])
        PL.p_log(stagealign_dir_name)
    # elif

    elif string1 == "C":
        log_string = "success_%d.raw" % (0)
        PL.p_log(log_string)
        stagealign_dir_name = "%s/polonator/G.007/acquisition/autoexp_FL_images/txred/" % (os.environ["HOME"])
        PL.p_log(stagealign_dir_name)
    # end elif

    elif string1 == "T":
        log_string =  "success_%d.raw" % (0)
        PL.p_log(log_string)

        stagealign_dir_name = "%s/polonator/G.007/acquisition/autoexp_FL_images/cy5/" % (os.environ["HOME"])
        PL.p_log(stagealign_dir_name)
    # end elif

    while i < num_array:
        # read and send the FL autoe images

        stagealign_rawimgfilename = "%simage_%d.raw" % (stagealign_dir_name, 60+i*10)
        log_string =  "STATUS:\t 1try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string)
        baseimage = numpy.fromfile(file=stagealign_rawimgfilename, dtype=np.uint16)
        
        log_string = "STATUS:\t 2try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string)

        blank_image = numpy.copy(baseimage)
        
        j = 0
        log_string =  "STATUS:\t 3try to send the autoexposure images %s to processor, port %d..." % (stagealign_rawimgfilename, proc_portnum)
        PL.p_log(log_string);

        while net.py_network_waitforsend() != 1:
            log_string = "Waiting to send the autoexposure images"
            PL.p_log(log_string);
        # end while
        
        log_string = "Trying to send autoexposure FL images %d" % (i)
        p_log_simple(log_string);
        net.py_network_sendimage(0, 0, i, blank_image)
        log_string = "Sent autoexposure FL images %d" % i
        p_log_simple(log_string)
        i += 1
    # end while

    i = 0

    while i < 2:
        i = net.py_network_waitforsend()
        log_string = "waiting to receive the gain value back"
        PL.p_log(log_string)
    # end while
    gain = (i - 97)*10 + 60

    log_string = "the autoexposure gain to use will be %d" %  (gain)
    p_log_simple(log_string)

    return gain
# end function

if __name__ == '__main__':
    main()
    
