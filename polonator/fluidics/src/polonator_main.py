#!/usr/bin/python

"""
--------------------------------------------------------------------------------
 Author: Richard Terry
 Date: March 19, 2008.
 For: G.007 polony sequencer design [software] at the Church Lab - 
 Genetics Department, Harvard Medical School.
 
 Purpose: This program contains the complete code for the iterative polony seq-
 uencing algorithm based on a given list of cycle-names in Python. It idles in 
 an infinite loop until the touch sensor is activated to run.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file. 
------------------------------------------------------------------------------- 
"""

import sys
import time
import getpass
import commands
import ConfigParser

from threading import Thread
from logger import Logger

from tel_net import Tel_net
sys.path.append("../")
# sys.path.append("../../G.007_acquisition/src")
import PolonatorImager
from biochem import Biochem

print '\nINFO\t ***\t*\t--> START POLONATOR MAIN - polonator_main.py'
print 'INFO\t ***\t*\t--> Please, slide your hand across touch sensor to ' + \
        'activate POLONATOR\n'

print 'ping', commands.getstatusoutput('ping -c 1 10.0.0.56')

error = 0
packet_loss = 0
while (error != (-1) and packet_loss != (-1)):
    ping_reply = commands.getstatusoutput('ping -c 1 10.0.0.56')
    find_string = ping_reply
    find_string = str(find_string)
    error = find_string.find('error')
    packet_loss = find_string.find('100% packet loss')
    time.sleep(10)

time.sleep(1)
session = Tel_net()
x_hmstat = 1
y_hmstat = 1

#while (x_hmstat != 0 and y_hmstat != 0):
#    x_hmstat = session.parse_read_string('x.hmstat', '>')
#    x_hmstat = x_hmstat.strip('>')
#    x_hmstat = x_hmstat.strip()
#    x_hmstat = int(x_hmstat)
#    y_hmstat = session.parse_read_string('y.hmstat', '>')
#    y_hmstat = y_hmstat.strip('>')
#    y_hmstat = y_hmstat.strip()
#    y_hmstat = int(y_hmstat)
#    time.sleep(0.1)

config = ConfigParser.ConfigParser()
config.readfp(open('config.txt'))
home_dir = config.get("communication","home_dir")

logger = Logger(config)         # initialize logger object
one_time_through=1

while (one_time_through==1):
    touch_sensor = \
        int(session.parse_read_string('m_din[7]', '>').strip('>').strip())

    # if (touch_sensor == 1):
    if (one_time_through==1):
        one_time_through = 0
        #commands.getstatusoutput('mplayer -ao alsa speech/welcome.wav')
        #commands.getstatusoutput('mplayer -ao alsa speech/run.wav')

        f = open('%s/cycle_list' % home_dir, 'r+')
        cycle_list = f.readlines()
        f.close()

        t0 = time.time()                # get current time
        logger.info("---\t-\t--> Started polony sequencing")


        if ((cycle_list[0] == 'WL1') or (cycle_list[0] == 'WL2')):
            w = open('%s/WL' % home_dir, 'w')
            w.write(int(cycle_list[0].strip('WL')))
            installed_flowcells = int(cycle_list[0].strip('WL'))
        else:
            w = open('%s/WL' % home_dir, 'r')
            installed_flowcells = int(w.read())

        w.close()
        logger.info("---\t-\t--> Number of installed_flowcells: %i" % \
            installed_flowcells)

        cycle_list_length = len(cycle_list)
        cycle_number = 0
        cycle_number_bio = 0
        flowcell = 0
        cycle_number_im = 0

        if (installed_flowcells == 1):
            for cycle_number in range(0, cycle_list_length):
                cycle_list[cycle_number] = cycle_list[cycle_number].strip()
                logger.info("---\t-\t--> Cycle_list key: %s" % \
                    cycle_list[cycle_number])
                logger.info("---\t-\t--> Cycle number: %i" % cycle_number)
                cycle_list2 = cycle_list


                # if(cycle_list[cycle_number] == 'abc'):
                if(1):
                    time.sleep(0.1)
                else:
                    # print cycle_list[cycle_number]
                    biochem = Biochem(cycle_list[cycle_number], flowcell, logger)
                    biochem.start()

                    while(biochem.isAlive()):
                        session.parse_read_string('y.ob[2]=1', '>')
                        session.parse_read_string('y.ob[2]=0', '>')
                        time.sleep(1)


                if(cycle_list[cycle_number] == 'abc'):
                    time.sleep(0.1)
                else:
                    # print cycle_list[cycle_number]
                    imager = PolonatorImager.Imager(cycle_list[cycle_number], \
                        flowcell)
                    imager.start()

                    while(imager.isAlive()):
                        session.parse_read_string('y.ob[1]=1', '>')
                        session.parse_read_string('y.ob[1]=0', '>')
                        time.sleep(1)


        if (installed_flowcells == 2):
            while (cycle_number <= cycle_list_length):
                logger.info("---\t-\t--> Cycle number: %i" % cycle_number)

                if (cycle_number == 0 and flowcell == 0):
                    cycle_list[cycle_number] = cycle_list[cycle_number].strip()
                    biochem = Biochem(cycle_list[cycle_number], flowcell, \
                        logger)
                    flowcell = int(not(flowcell))
                    biochem.start()

                elif (cycle_number < cycle_list_length):
                    cycle_list[cycle_number] = cycle_list[cycle_number].strip()
                    biochem = Biochem(cycle_list[cycle_number], flowcell, logger)
                    imager = PolonatorImager.Imager(cycle_list[cycle_number_im], \
                        int(not(flowcell)))
                    biochem.start()
                    imager.start()

                    if (flowcell == 1 and cycle_number < cycle_list_length):
                        cycle_number_im = cycle_number
                        cycle_number = cycle_number + 1
                        flowcell = int(not(flowcell))
                    else:
                        flowcell = int(not(flowcell))
                        cycle_number_im = cycle_number

                elif (cycle_number == cycle_list_length):
                    imager = PolonatorImager.Imager(cycle_list[cycle_number_im], \
                         int(not(flowcell)))
                    imager.start()
                    cycle_number = cycle_number + 1

                session.parse_read_string('y.ob[1]=0', '>')
                session.parse_read_string('y.ob[2]=0', '>')

                if (cycle_number == 0 and flowcell == 1):
                    while(biochem.isAlive()):
                        if (biochem.isAlive()):
                            session.parse_read_string('y.ob[2]=1', '>')
                            time.sleep(0.1)
                else:
                    while(imager.isAlive() or biochem.isAlive()):
                        if (imager.isAlive()):
                            session.parse_read_string('y.ob[1]=1', '>')
                            time.sleep(0.1)

                        if (biochem.isAlive()):
                            session.parse_read_string('y.ob[2]=1', '>')
                            time.sleep(0.1)
                time.sleep(0.1)

    session.parse_read_string('y.ob[1]=0', '>')
    session.parse_read_string('y.ob[2]=0', '>')
    time.sleep(0.1)

# calculate elapsed time for polony sequencing cycles
delta = (time.time() - t0) / 60
logger.warn(("***\t*\t--> Finished polony sequencing - duration: " + \ 
            "%0.2f minutes\n") % delta)

