"""

------------------------------------------------------------------------------
 Author: Mirko Palla.
 Date: April 29, 2008.
 Modified: Richard Terry
 Date: July 11, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab -
 Genetics Department, Harvard Medical School.

 Purpose: This program contains the complete code for module Biochem,
 containing re-occurring biochecmistry subroutines in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------
Modified by Greg Porreca 01-23-2009 to add a user-friendly temperature control
method "set_temp", and support for Illumina sequencing chemistry.  Significant
tidying-up throughout, including removal of misleading comments, etc.  Changed
logging throughout to be less obtuse.

Modified by Richard Terry Jun-10-2009 to fix bugs and change custom experiment
EXP

"""

import os
import sys
import time
import commands
import math

import ConfigParser
import logging
from threading import Thread

from serial_port import Serial_port

# supports serial port multiplexing to address multiple hardware devices
from mux import Mux
# interface to syringe pump and associated valve
from syringe_pump import Syringe_pump
# interface to a single rotary valve; the valve in use is
# specified by the mux state
from rotary_valve import Rotary_valve
# interface to single temp controller; controller in use is
# specified by mux state
from temperature_control import Temperature_control


"""
start the logger; dump all messages to a file, and INFO and higher to stdout
the same logger is used by the whole biochem system, but different modules
log differently (i.e. a 'name' stamp describes whether the output is from
'biochem', or from one of the hadware sub-modules)
"""
logging.basicConfig(level=logging.DEBUG, \
                    format='%(asctime)s: %(name)s: %(levelname)s:\t%(message)s',
                    filename= os.environ['HOME'] + '/polonator/G.007/acquisition/logs/fluidics/biochem.log',
                    filemode='a')
console=logging.StreamHandler(sys.stdout)
console.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s: %(name)s\t%(levelname)s:\t%(message)s')
console.setFormatter(formatter)
logging.getLogger('').addHandler(console)

# this file will log as 'biochem'
log = logging.getLogger('biochem')


class Biochem(Thread):

    def __init__(self, cycle_name, flowcell, logger=None):
        "Initialize biochemistry object with default parameters."

        self.state = 'biochemistry object'

        # set cycle-name parameter to specified value (4 chars)
        self.cycle_name = cycle_name
        # set cycle parameter to specified value (first 3 chars)
        self.cycle = cycle_name[0:3]
        self.flowcell = flowcell

        self.config = ConfigParser.ConfigParser()
        self.config.readfp(open(os.environ['POLONATOR_PATH'] + '/config_files/sequencing.cfg'))

        Thread.__init__(self)  # instantiate thread

        self.mux = Mux()
        self.ser = Serial_port(self.config)

        self.rotary_valve = Rotary_valve(self.config, self.ser, self.mux)
        self.syringe_pump = Syringe_pump(self.config, self.ser, self.mux)
        self.temperature_control = Temperature_control(self.config, self.ser)

        # retrieve all configuatrion parameters from file
        self.get_config_parameters()
        log.info("%s\t%i\t--> Biochemistry object is constructed: [%s]" % \
            (self.cycle_name, self.flowcell, self.state) )


    """
    COMPLEX FUNCTIONS
    """


    """
    Get_config_parameters
    """

    def setCycleName(self, cyc_name):
        self.cycle_name = cyc_name


    def get_config_parameters(self):
        """
        Retieves all biochemistry and device related configuration parameters
        from the configuration file using the ConfigParser facility. It assigns
        each parameter to a field of the biochemistry object, thus it can access
        it any time during a run.
        """

        log.info( ("%s\t%i\t--> Retrieve configuration parameters from " + \
                "file: [%s]") % \
                (self.cycle_name, self.flowcell, self.state))

        """
        Tubing configuration
        """

        self.channel_volume = int(self.config.get("tube_constants", \
                                                    "channel_volume"))
        self.flowcell_volume = int(self.config.get("tube_constants", \
                                                    "flowcell_volume"))

        self.dH2O_to_V4 = int(self.config.get("tube_constants","dH2O_to_V4"))
        self.wash1_to_V = int(self.config.get("tube_constants","wash1_to_V"))
        self.V_to_V4 = int(self.config.get("tube_constants","V_to_V4"))
        self.V4_to_T = int(self.config.get("tube_constants","V4_to_T"))
        self.V5_to_T = int(self.config.get("tube_constants","V5_to_T"))
        self.ligase_to_V5 = int(self.config.get("tube_constants","ligase_to_V5"))
        self.T_to_Y = int(self.config.get("tube_constants","T_to_Y"))
        self.Y_to_FC = int(self.config.get("tube_constants","Y_to_FC"))
        self.FC_to_syringe = int(self.config.get("tube_constants", \
                                                    "FC_to_syringe"))
        self.well_to_V = int(self.config.get("tube_constants","well_to_V"))
        self.NaOH_to_V4 = int(self.config.get("tube_constants","NaOH_to_V4"))
        self.guadinine_to_V4 = int(self.config.get("tube_constants", \
                                                    "guadinine_to_V4"))

        """
        Reagent block chamber configuration
        """

        self.ligase_chamber_volume = int(self.config.get("block_constants",\
                                        "ligase_chamber_volume"))

        """
        Syringe configuration
        """

        self.full_stroke = int(self.config.get("syringe_constants","full_stroke"))
        self.pull_speed = int(self.config.get("syringe_constants","pull_speed"))
        self.slow_speed = int(self.config.get("syringe_constants","slow_speed"))
        self.fast_speed = int(self.config.get("syringe_constants","fast_speed"))
        self.final_pull_speed = int(self.config.get("syringe_constants",\
                                                    "final_pull_speed") )
        self.empty_speed = int(self.config.get("syringe_constants","empty_speed"))

        """
        Biochem parameters
        """

        self.stage_temp = int(self.config.get("biochem_parameters","stage_temp"))
        self.room_temp = int(self.config.get("biochem_parameters","room_temp"))
        self.temp_tolerance = int(self.config.get("biochem_parameters",\
                                                        "temp_tolerance"))
        self.air_gap = int(self.config.get("biochem_parameters","air_gap"))
        self.time_limit = int(self.config.get("biochem_parameters","time_limit"))
        self.slow_push_volume = int(self.config.get("biochem_parameters", \
                                                            "slow_push_volume"))

        """
        Enzymatic reaction parameters
        """

        self.exo_volume = int(self.config.get("exo_parameters","exo_volume"))
        self.exo_temp = int(self.config.get("exo_parameters","exo_temp"))
        self.exo_set_temp = int(self.config.get("exo_parameters","exo_set_temp"))
        self.exo_poll_temp = int(self.config.get("exo_parameters","exo_poll_temp"))
        self.exo_time = int(self.config.get("exo_parameters","exo_time"))

        self.exo_extra = int(self.config.get("exo_parameters","exo_extra"))

        """
        Stripping parameters
        """

        self.guadinine_volume = int(self.config.get("stripping_parameters", \
                                                            "guadinine_volume"))
        self.NaOH_volume = int(self.config.get("stripping_parameters", \
                                                                "NaOH_volume"))
        self.dH2O_volume = int(self.config.get("stripping_parameters",\
                                                                "dH2O_volume"))

        self.guadinine_time = int(self.config.get("stripping_parameters",\
                                                            "guadinine_time"))
        self.NaOH_time = int(self.config.get("stripping_parameters","NaOH_time"))

        self.guadinine_extra = int(self.config.get("stripping_parameters",\
                                                            "guadinine_extra"))
        self.NaOH_extra = int(self.config.get("stripping_parameters",\
                                                                  "NaOH_extra"))

        """
        Hybridization parameters
        """

        self.primer_volume = int(self.config.get("hyb_parameters", \
                                                               "primer_volume"))

        self.hyb_temp1 = int(self.config.get("hyb_parameters","hyb_temp1"))
        self.hyb_set_temp1 = int(self.config.get("hyb_parameters",\
                                                               "hyb_set_temp1"))
        self.hyb_poll_temp1 = int(self.config.get("hyb_parameters", \
                                                               "hyb_poll_temp1"))
        self.hyb_time1 = int(self.config.get("hyb_parameters","hyb_time1"))

        self.hyb_temp2 = int(self.config.get("hyb_parameters","hyb_temp2"))
        self.hyb_set_temp2 = int(self.config.get("hyb_parameters", \
                                                               "hyb_set_temp2"))
        self.hyb_poll_temp2 = int(self.config.get("hyb_parameters",\
                                                              "hyb_poll_temp2"))
        self.hyb_time2 = int(self.config.get("hyb_parameters","hyb_time2"))

        self.hyb_extra = int(self.config.get("hyb_parameters","hyb_extra"))

        """
        Ligation parameters
        """

        self.buffer_volume = int(self.config.get("lig_parameters", \
                                                    "buffer_volume"))
        self.ligase_volume = int(self.config.get("lig_parameters", \
                                                    "ligase_volume"))
        self.nonamer_volume = int(self.config.get("lig_parameters", \
                                                     "nonamer_volume"))
        self.reagent_volume = self.ligase_volume + self.nonamer_volume

        self.lig_step1 = int(self.config.get("lig_parameters","lig_step1"))
        self.lig_set_step1 = int(self.config.get("lig_parameters", \
                                                    "lig_set_step1"))
        self.lig_poll_step1 = int(self.config.get("lig_parameters", \
                                                    "lig_poll_step1"))
        self.lig_time1 = int(self.config.get("lig_parameters","lig_time1"))

        self.lig_step2 = int(self.config.get("lig_parameters","lig_step2"))
        self.lig_set_step2 = int(self.config.get("lig_parameters", \
                                                    "lig_set_step2"))
        self.lig_poll_step2 = int(self.config.get("lig_parameters", \
                                                            "lig_poll_step2"))
        self.lig_time2 = int(self.config.get("lig_parameters","lig_time2"))

        self.lig_step3 = int(self.config.get("lig_parameters","lig_step3"))
        self.lig_set_step3 = int(self.config.get("lig_parameters",\
                                                               "lig_set_step3"))
        self.lig_poll_step3 = int(self.config.get("lig_parameters", \
                                                              "lig_poll_step3"))
        self.lig_time3 = int(self.config.get("lig_parameters","lig_time3"))

        self.lig_step4 = int(self.config.get("lig_parameters","lig_step4"))
        self.lig_set_step4 = int(self.config.get("lig_parameters", \
                                                               "lig_set_step4"))
        self.lig_poll_step4 = int(self.config.get("lig_parameters", \
                                                              "lig_poll_step4"))
        self.lig_time4 = int(self.config.get("lig_parameters","lig_time4"))

        self.mix_time = int(self.config.get("lig_parameters","mix_time"))
        self.lig_extra = int(self.config.get("lig_parameters","lig_extra"))

        """
        Cycle constants
        """

        self.port_scheme = eval(self.config.get("cycle_constants","port_scheme"))

        """
        Path lenghts / volumes
        """

        self.V_to_FC_end = self.V_to_V4 + self.V4_to_T + \
                            self.T_to_Y + self.Y_to_FC + self.channel_volume
        self.V4_to_FC_end = self.V4_to_T + self.T_to_Y + \
                                        self.Y_to_FC + self.channel_volume
        # total of 3 (1+2) flowcell volumes
        self.FC_wash = self.V4_to_FC_end + 2 * self.flowcell_volume

    """
    Gap volume calculation
    """

    def gap_volume(self, reagent_volume):
        """
        Determines the positioning gap needed to center the reagent volume
        in the flowcell.
        """

        gap_volume = int((reagent_volume - self.flowcell_volume) / 2)#RCT
        return gap_volume

    """
    V to-V4 cleaning
    """

    def clean_V_to_V4(self, rotary_valve):
        """
        Fills tube path V to V4 with Wash1 and dumps previous tube content
        to waste.
        """

        log.info("%s\t%i\t--> Clean V to V4 [%s]" % (self.cycle_name, \
             self.flowcell, self.state))

        if(rotary_valve == 'V1'):
            #RCT switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve1()
              #RCT set rotary valve to position 9
            self.rotary_valve.set_valve_position(9)
            #RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            #RCT set rotary valve to position 1
            self.rotary_valve.set_valve_position(1)
        elif(rotary_valve == 'V2'):
            #RCT switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve2()
            #RCT set rotary valve to position 9
            self.rotary_valve.set_valve_position(9)
            #RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            #RCT set rotary valve to position 2
            self.rotary_valve.set_valve_position(2)
        elif(rotary_valve == 'V3'):
            #RCT switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve3()
            #RCT set rotary valve to position 9
            self.rotary_valve.set_valve_position(9)
            #RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            #RCT set rotary valve to position 3
            self.rotary_valve.set_valve_position(3)

        log.info("%s\t%i\t--> Draw %i ul Wash 1 up to V4" % \
                (self.cycle_name, self.flowcell, self.V_to_V4, 1))
        #RCT draw wash up to syringe pump port 1 and eject tube content to waste
        self.move_reagent(self.V_to_V4, self.pull_speed, 1, self.empty_speed, 3)

    """
    Flowcell flushing
    """

    def flush_flowcell(self, V4_port):
        """
        Flushes flowcell with 'Wash' or dH2O.
        """

        log.info("%s\t%i\t--> Flush flowcell from port %s: [%s]" % \
                (self.cycle_name, self.flowcell, V4_port, self.state))

        if self.flowcell == 0:
            from_port = 1  #RCT2 # set syringe pump port variable to 2

        else:
            from_port = 2  #RCT3 # set syringe pump port variable to 3

        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V3 to designated port
        self.rotary_valve.set_valve_position(V4_port)

        log.info("%s\t%i\t--> Flush flowcells fast (%i ul) and eject to waste" % \
                (self.cycle_name, self.flowcell, self.FC_wash*5))
        #RCT flush flowcells 4-times and eject to waste
        self.move_reagent(self.FC_wash*5, 24, from_port, self.empty_speed, 3)
        #RCT flush flowcells 4-times and eject to waste
        # self.move_reagent(self.FC_wash*5, 29, from_port, self.empty_speed, 3)

        log.info("%s\t%i\t--> Flush flowcells (%i ul) and eject to waste" % \
                (self.cycle_name, self.flowcell, self.FC_wash*3))
         #RCT flush flowcells 3-times and eject to waste
        self.move_reagent(self.FC_wash*3, self.fast_speed, \
                            from_port, self.empty_speed, 3)

    """
    Draw reagent into flowcell
    """

    def draw_into_flowcell(self, draw_port, rotary_valve, rotary_port, reagent_volume):
        """
        Draws reagent into flowcell.
        """

        log.info("%s\t%i\t--> Draw %i ul of reagent into flowcell %i: [%s]" % \
                    (self.cycle_name, self.flowcell, reagent_volume, \
                        self.flowcell, self.state))

        if self.flowcell == 0:
            from_port = 1 # set syringe pump port variable to 2
        else:
            from_port = 2 # set syringe pump port variable to 3

        if (rotary_valve == 'V4'):
            FC_draw = self.V4_to_FC_end
        else:
            FC_draw = self.V_to_FC_end

        self.draw_reagent(rotary_valve, rotary_port, reagent_volume)

        if rotary_valve == 'V1':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(1)
            # switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve1()
        elif rotary_valve == 'V2':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(2)
            # switch communication to ten port rotary valve V2
            self.mux.set_to_rotary_valve2()
        elif rotary_valve == 'V3':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(3)
            # switch communication to ten port rotary valve V3
            self.mux.set_to_rotary_valve3()
        elif rotary_valve == 'V4':
            self.mux.set_to_rotary_valve4()

        self.rotary_valve.set_valve_position(draw_port)
        log.info("%s\t%i\t--> Push reagent into flowcell with %i ul of Wash" % \
                (self.cycle_name, self.flowcell, \
                    FC_draw-self.gap_volume(reagent_volume) - self.air_gap +0))
        # RCT do iterative flushes with last 200 ul stroke at slow
        # syringe speed and eject to waste
        self.move_reagent_slow(FC_draw - self.gap_volume(reagent_volume) - \
                                self.air_gap +0, self.pull_speed, \
                                from_port, self.empty_speed, 3)


    def draw_into_flowcell_bufferchase(self, draw_port, \
                                            rotary_valve, \
                                            rotary_port, \
                                            reagent_volume, \
                                            bufferport, \
                                            buffervol):

        log.info("%s\t%i\t--> Draw %i ul of reagent into flowcell w/ buffer " + \
                "chase (port %d, volume %d ul):" % \
                (self.cycle_name, self.flowcell, reagent_volume, \
                                                        bufferport, buffervol))

        if self.flowcell == 0:
            from_port = 1 # set syringe pump port variable to 2
        else:
            from_port = 2 # set syringe pump port variable to 3

        if (rotary_valve == 'V4'):
            FC_draw = self.V4_to_FC_end
        else:
            FC_draw = self.V_to_FC_end

        # calculate amount of buffer to pull after slug to center slug in flowcell
        move_distance = FC_draw - \
                        self.gap_volume(reagent_volume) - \
                        self.air_gap +0
        if buffervol > move_distance:
            log.info( "ERROR: buffer volume %d ul specified, max allowed is" + \
                        " %d; will only use %d ul\n" % \
                        (buffervol, move_distance, move_distance))
            buffervol = move_distance
        move_distance = move_distance - buffervol


        self.draw_reagent(rotary_valve, rotary_port, reagent_volume)
        self.draw_reagent(rotary_valve, bufferport, buffervol)

        if rotary_valve == 'V1':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(1)
            # switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve1()
        elif rotary_valve == 'V2':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(2)
            # switch communication to ten port rotary valve V2
            self.mux.set_to_rotary_valve2()
        elif rotary_valve == 'V3':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(3)
            # switch communication to ten port rotary valve V3
            self.mux.set_to_rotary_valve3()
        elif rotary_valve == 'V4':
            # switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()

        self.rotary_valve.set_valve_position(draw_port)
        log.info("%s\t%i\t--> Push reagent into flowcell with %i ul of Wash" % \
                (self.cycle_name, self.flowcell, move_distance))
        # RCT do iterative flushes with last 200 ul stroke at slow syringe speed
        # and eject to waste
        self.move_reagent_slow(move_distance, self.pull_speed, \
                                from_port, self.empty_speed, 3)




    """
    Reagent transfer through syringe
    """

    def move_reagent(self, fill_volume, from_speed, \
                            from_port, to_speed, to_port):
        """
        Moves a given volume of reagent [1] into syringe at speed [2] through
        specified valve position [3], then transfers syringe content through
        valve position [4] into an other location in the fluidic system.
        All parameters biochem.pyare integers respectively.
        """

        if fill_volume != 0:
            # switch communication to nine port syringe pump
            self.mux.set_to_syringe_pump()

            if fill_volume <= self.full_stroke:
                # draw into syringe
                self.syringe_pump.set_speed(from_speed)
                self.syringe_pump.set_valve_position(from_port)
                self.syringe_pump.set_absolute_volume(fill_volume)

                # transfer fluid through 'to_port'
                self.syringe_pump.set_speed(to_speed)
                if from_port == 1:
                    self.syringe_pump.set_valve_position_CCW(to_port)
                else:
                    self.syringe_pump.set_valve_position(to_port)
                self.syringe_pump.set_absolute_volume(0)

            else:
                iteration = int(fill_volume / self.full_stroke)
                remainder = int(fill_volume - (iteration * self.full_stroke))

                for i in range(0, iteration):

                    self.syringe_pump.set_speed(from_speed)  # draw into syringe
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(self.full_stroke)
                    # transfer fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)

                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)
                    self.syringe_pump.set_absolute_volume(0)

                if remainder != 0:
                    # draw remainder into syringe
                    self.syringe_pump.set_speed(from_speed)
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(remainder)
                    # transfer remainder fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)
                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)
                    self.syringe_pump.set_absolute_volume(0)

    """
    Slow reagent transfer through syringe
    """

    def move_reagent_slow(self, fill_volume, from_speed, \
                                from_port, to_speed, to_port):
        """
        Moves a given volume of reagent [1] into syringe at speed [2] through
        specified valve position [3], then transfers syringe content through
        valve position [4] into an other location in the fluidic system.
        The last 200 ul reagent is drawn into the flowcell with slower speed to
        avoid air bubble build up in the chambers. All parameters are integers
        respectively.
        """

        if fill_volume != 0:
            # switch communication to nine port syringe pump
            self.mux.set_to_syringe_pump()

            if fill_volume <= self.full_stroke:
                # if flowcell-fill volume less than last slow-fill volume
                if fill_volume <= self.slow_push_volume:

                    # Slow push in to aviod air bubbles
                    # draw into syringe
                    self.syringe_pump.set_speed(self.final_pull_speed)
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(fill_volume)
                    # transfer fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)
                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)

                    self.syringe_pump.set_absolute_volume(0)
                else:
                    #  calculate first push volume
                    first_push_volume = fill_volume - self.slow_push_volume

                    self.syringe_pump.set_speed(from_speed)  # draw into syringe
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(first_push_volume)

                    # transfer fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)
                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)

                    self.syringe_pump.set_absolute_volume(0)

                    # Slow push in to aviod air bubbles
                    if self.slow_push_volume != 0:
                        # draw into syringe
                        self.syringe_pump.set_speed(self.final_pull_speed)
                        self.syringe_pump.set_valve_position(from_port)
                        self.syringe_pump.set_absolute_volume(self.slow_push_volume)
                        # transfer fluid through 'to_port'
                        self.syringe_pump.set_speed(to_speed)
                        if from_port == 1:
                            self.syringe_pump.set_valve_position_CCW(to_port)
                        else:
                            self.syringe_pump.set_valve_position(to_port)

                        self.syringe_pump.set_absolute_volume(0)
            else:
                #  calculate first push volume
                first_push_volume = fill_volume - self.slow_push_volume

                iteration = int(first_push_volume / self.full_stroke)
                remainder = int(first_push_volume - (iteration * self.full_stroke))

                for i in range(0, iteration):

                    self.syringe_pump.set_speed(from_speed)  # draw into syringe
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(self.full_stroke)
                    # transfer fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)
                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)

                    self.syringe_pump.set_absolute_volume(0)

                if remainder != 0:
                    # draw remainder into syringe
                    self.syringe_pump.set_speed(from_speed)
                    self.syringe_pump.set_valve_position(from_port)
                    self.syringe_pump.set_absolute_volume(remainder)
                    # transfer remainder fluid through 'to_port'
                    self.syringe_pump.set_speed(to_speed)
                    if from_port == 1:
                        self.syringe_pump.set_valve_position_CCW(to_port)
                    else:
                        self.syringe_pump.set_valve_position(to_port)

                    self.syringe_pump.set_absolute_volume(0)

                    # Slow push in to aviod air bubbles

                    if self.slow_push_volume != 0:
                        # draw into syringe
                        self.syringe_pump.set_speed(self.final_pull_speed)
                        self.syringe_pump.set_valve_position(from_port)
                        self.syringe_pump.set_absolute_volume(self.slow_push_volume)
                        # transfer fluid through 'to_port'
                        self.syringe_pump.set_speed(to_speed)
                        if from_port == 1:
                            self.syringe_pump.set_valve_position_CCW(to_port)
                        else:
                            self.syringe_pump.set_valve_position(to_port)

                        self.syringe_pump.set_absolute_volume(0)

    """
    Air gap drawing to valves
    """

    def draw_air_to_valve(self, valve, gap_size=None):
        """
        Draws a 30 ul volume of air plug in front of specified valve COM-port
        assuming that all V10 ports open to air.
        """


        if gap_size is None:
            # if no argument given, use default air gap size
            gap_size = self.air_gap

        if gap_size == 0:
            time.sleep(0.001)

        else:
            log.info("%s\t%i\t--> Draw air bubble to valve %s COM-port: [%s]" % \
                    (self.cycle_name, self.flowcell, valve, self.state))
            if valve == 'V1':
                # switch communication to ten port rotary valve V4
                self.mux.set_to_rotary_valve4()
                self.rotary_valve.set_valve_position(1)
                # switch communication to ten port rotary valve V1
                self.mux.set_to_rotary_valve1()
                self.rotary_valve.set_valve_position(10)

            elif valve == 'V2':
                # switch communication to ten port rotary valve V4
                self.mux.set_to_rotary_valve4()
                self.rotary_valve.set_valve_position(2)
                # switch communication to ten port rotary valve V2
                self.mux.set_to_rotary_valve2()
                # switch rotary valve V2 to port 10
                self.rotary_valve.set_valve_position(10)

            elif valve == 'V3':
                # switch communication to ten port rotary valve V4
                self.mux.set_to_rotary_valve4()
                self.rotary_valve.set_valve_position(3)
                # switch communication to ten port rotary valve V3
                self.mux.set_to_rotary_valve3()
                # switch rotary valve V3 to port 10
                self.rotary_valve.set_valve_position(10)

            elif valve == 'V4':
                # switch communication to ten port rotary valve V2
                self.mux.set_to_rotary_valve4()
                # switch rotary valve V2 to port 10
                self.rotary_valve.set_valve_position(10)

            if self.flowcell == 0:
                from_port = 1 # set syringe pump port variable to 1

            else:
                from_port = 2 # set syringe pump port variable to 3

            log.info("%s\t%i\t--> Draw %i ul air gap in front of COM-port" % \
                        (self.cycle_name, self.flowcell, gap_size))
            self.move_reagent(gap_size, self.pull_speed, from_port, \
                    self.empty_speed, 3)  # draw air gap in front of COM-port

    """
    Room temperature setting
    """

    def set_to_RT(self):
        """
        Sets temperataure controller to room temperature (self.room_temp C).
        """

        log.info("%s\t%i\t--> Set temperature controller to %i C: [%s]" % \
            (self.cycle_name, self.flowcell, self.room_temp, self.state))

        if self.flowcell == 0:
            # set communication to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set communication to temperature controller 2
            self.mux.set_to_temperature_control2()
        # set temperature controller to 30 C
        self.temperature_control.set_temperature(self.room_temp)

        t0 = time.time()  # get current time
        # get current flowcell temperature
        tc = self.temperature_control.get_temperature()

        while abs(self.room_temp - tc) > 2:

            tc = self.temperature_control.get_temperature()
            delta = time.time() - t0 # elapsed time in seconds

            if delta > self.time_limit * 60:
                break

        sys.stdout.write("TIME\t ---\t-\t--> Elapsed time: %i s and current " + \
                        "temperature: %0.2f C\n" % (int(delta), tc))

    """
    Steady-state temperature waiting
    """

    def wait_for_SS(self, set_temp, poll_temp, tolerance=None):
        """
        Waits until steady-state temperature is reached, or exits wait
        block if ramping time exceeds timeout parameter set in configuration
        file.
        """

        log.info("%s\t%i\t--> Wait for steady-state - poll temperature %i C, " + \
                "set_temp %d: [%s]\n" % \
                (self.cycle_name, self.flowcell, poll_temp, \
                    set_temp, self.state))

        # if temperature tolerance is not defined, set default to +/- 1 C
        if tolerance is None:
            tolerance = 1

        t0 = time.time()  # get current time
        # get current flowcell temperature
        tc = self.temperature_control.get_temperature()

        if set_temp - poll_temp >= 0:  # if ramping up
            if poll_temp >= tc:
                while abs(poll_temp - tc) > tolerance:

                    tc = self.temperature_control.get_temperature()
                    time.sleep(1)

                    delta = time.time() - t0 # elapsed time in seconds
                    if delta > self.time_limit * 60:
                        log.warn( ("%s\t%i\t --> Time limit %s exceeded -> " + \
                            "[current: %0.2f, target: %0.2f] C: [%s]") % \
                            (self.cycle_name, self.flowcell, self.time_limit, \
                                tc, poll_temp, self.state))
                        break

            sys.stdout.write("TIME\t ---\t-\t--> Elapsed time: %i s and " + \
                            "current temperature: %0.2f C\n" % (int(delta), tc))
        else:  # if ramping down
            if poll_temp <= tc:
                while abs(poll_temp - tc) > tolerance:

                    tc = self.temperature_control.get_temperature()
                    time.sleep(1)

                    delta = time.time() - t0 # elapsed time in seconds
                    sys.stdout.flush()

                    if delta > self.time_limit * 60:
                        log.warn( ("%s\t%i\t --> Time limit %s exceeded -> " + \
                            "[current: %0.2f, target: %0.2f] C: [%s]" ) %  \
                            (self.cycle_name, self.flowcell, self.time_limit, \
                                tc, poll_temp, self.state))
                        break
        print '\n'
        elapsed = (time.time() - t0) / 60

        log.warn( ("%s\t%i\t--> Time to set steady-state temperature: " + \
                    "%0.2f minutes and current temperature: %0.2f C: [%s]\r") % \
                    (self.cycle_name, self.flowcell, elapsed, tc, self.state))



    """
    Incubate and count elapsed time
    """

    def incubate_reagent(self, time_m):
        """
        Incubates reagent for given amount of time and dynamically counts
        elapsed time in seconds to update user about incubation state.
        """

        log.info("%s\t%i\t--> Incubate reagent for %i min: [%s]" %  \
            (self.cycle_name, self.flowcell, time_m, self.state))

        incubation_time = time_m * 60  # incubation time in seconds

        for tc in range(0, incubation_time):

            time.sleep(1)

    """
    Draw reagent
    """

    def draw_reagent(self, rotary_valve, rotary_port, reagent_volume):
        """
        Moves nonamer up to V com.
        """

# DONT EVER DO THIS ON THE POLONATOR G.007
#        self.draw_air_to_valve(rotary_valve)

#        self.rotary_valve.usePort(rotary_valve, rotary_port)


        if rotary_valve == 'V1':
            # RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(1)
            # RCT switch communication to ten port rotary valve V1
            self.mux.set_to_rotary_valve1()
            self.rotary_valve.set_valve_position(rotary_port)

        elif rotary_valve == 'V2':
            # RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(2)
            # RCT switch communication to ten port rotary valve V2
            self.mux.set_to_rotary_valve2()
            self.rotary_valve.set_valve_position(rotary_port)

        elif rotary_valve == 'V3':
            # RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(3)
            # RCT switch communication to ten port rotary valve V3
            self.mux.set_to_rotary_valve3()
            self.rotary_valve.set_valve_position(rotary_port)

        elif rotary_valve == 'V4':
            # RCT switch communication to ten port rotary valve V4
            self.mux.set_to_rotary_valve4()
            self.rotary_valve.set_valve_position(rotary_port)

        self.move_reagent(reagent_volume, self.slow_speed, \
            self.flowcell+1, self.empty_speed, 3)  #RCT pull reagent volume

# DONT EVER DO THIS ON THE POLONATOR G.007
#        self.draw_air_to_valve(rotary_valve)

    """
    Ligase mixing
    """

    def ligase_mix(self, mix_time):
        """
        Mixes ligase with nonamer
        """

        log.info( ("%s\t%i\t--> NOT ACTUALLY DOING ANYTHING Mix reagent " + \
                    "in mixing chamber for %i seconds: [%s]") % \
                    (self.cycle_name, self.flowcell, mix_time, self.state))


    """
    PRIMING FUNCTIONS
    """

    """
    Prime rotary valve 1 chambers
    """

    def prime_rotary_valve1(self):
        """
        Primes reagent block chambers in ten port rotary valve V1
        """

        log.info("%s\t%i\t--> Prime rotary valve V1 reagent block chambers: [%s]" \
        % (self.cycle_name, self.flowcell, self.state))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 1
        self.rotary_valve.set_valve_position(1)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 1
        self.rotary_valve.set_valve_position(1)
        log.info("%s\t%i\t--> Prime position 1 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 2
        self.rotary_valve.set_valve_position(2)
        log.info("%s\t%i\t--> Prime position 2 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 3
        self.rotary_valve.set_valve_position(3)
        log.info("%s\t%i\t--> Prime position 3 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 4
        self.rotary_valve.set_valve_position(4)
        log.info("%s\t%i\t--> Prime position 4 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 5
        self.rotary_valve.set_valve_position(5)
        log.info("%s\t%i\t--> Prime position 5 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 6
        self.rotary_valve.set_valve_position(6)
        log.info("%s\t%i\t--> Prime position 6 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 7
        self.rotary_valve.set_valve_position(7)
        log.info("%s\t%i\t--> Prime position 7 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 8
        self.rotary_valve.set_valve_position(8)
        log.info("%s\t%i\t--> Prime position 8 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 10
        self.rotary_valve.set_valve_position(9)
        log.info("%s\t%i\t--> Draw %i ul Wash 1 up to V1-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.wash1_to_V))
        self.move_reagent(5 * self.wash1_to_V, self.fast_speed, 1, \
            self.empty_speed, 3) # draw Wash 1 up to V1-10

    """
    Prime rotary valve 2 chambers
    """

    def prime_rotary_valve2(self):
        """
        Primes reagent block chambers in ten port rotary valve V2.
        """
        log.info("%s\t%i\t--> Prime rotary valve V2 reagent block " + \
            "chambers: [%s]" % (self.cycle_name, self.flowcell, self.state))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 2
        self.rotary_valve.set_valve_position(2)


        log.info("%s\t%i\t--> Prime position 1 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 1
        self.rotary_valve.set_valve_position(1)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
                            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 2 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 2
        self.rotary_valve.set_valve_position(2)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 3 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 3
        self.rotary_valve.set_valve_position(3)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 4 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 4
        self.rotary_valve.set_valve_position(4)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 5 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 5
        self.rotary_valve.set_valve_position(5)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 6 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 6
        self.rotary_valve.set_valve_position(6)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 7 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 7
        self.rotary_valve.set_valve_position(7)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 8 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 8
        self.rotary_valve.set_valve_position(8)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Draw %i ul Wash 1 up to V1-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.wash1_to_V))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 10
        self.rotary_valve.set_valve_position(9)
        self.move_reagent(5 * self.wash1_to_V, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT draw Wash1 up to V2-10

    """
    Prime rotary valve 3 chambers
    """

    def prime_rotary_valve3(self):
        """
        Primes reagent block chambers in ten port rotary valve V3.
        """
        log.info(("%s\t%i\t--> Prime rotary valve V3 reagent block " + \
            "chambers: [%s]") % (self.cycle_name, self.flowcell, self.state))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 3
        self.rotary_valve.set_valve_position(3)


        log.info("%s\t%i\t--> Prime position 1 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 1
        self.rotary_valve.set_valve_position(1)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 2 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 2
        self.rotary_valve.set_valve_position(2)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 3 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 3
        self.rotary_valve.set_valve_position(3)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 4 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 4
        self.rotary_valve.set_valve_position(4)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 5 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 5
        self.rotary_valve.set_valve_position(5)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 6 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 6
        self.rotary_valve.set_valve_position(6)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 7 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 7
        self.rotary_valve.set_valve_position(7)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 8 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.well_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 8
        self.rotary_valve.set_valve_position(8)
        self.move_reagent(self.well_to_V*10, self.fast_speed, 1, \
            self.empty_speed, 3)

        log.info("%s\t%i\t--> Draw %i ul Wash 1 up to V3-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.wash1_to_V))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 10
        self.rotary_valve.set_valve_position(9)
        self.move_reagent(5 * self.wash1_to_V, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT draw Wash1 up to V3-10

    """
    Prime rotary valve 4 chambers
    """

    def prime_rotary_valve4(self):
        """
        Primes reagent block chambers in ten port rotary valve V3.
        """

        log.info(("%s\t%i\t--> Prime rotary valve V4 reagent block chambers:" + \
                " [%s]") % (self.cycle_name, self.flowcell, self.state))

        log.info("%s\t%i\t--> Prime position 1 with %i ul pre-loaded fluid" \
                % (self.cycle_name, self.flowcell, self.V_to_V4))
        # switch communication to ten port rotary valve V1
        self.mux.set_to_rotary_valve1()
        # switch rotary valve V1 to port 9
        self.rotary_valve.set_valve_position(9)
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V3 to port 1
        self.rotary_valve.set_valve_position(1)
        self.move_reagent(self.V_to_V4, self.fast_speed, 1, self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 2 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.V_to_V4))
        # switch communication to ten port rotary valve V2
        self.mux.set_to_rotary_valve2()
        # switch rotary valve V2 to port 9
        self.rotary_valve.set_valve_position(9)
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 2
        self.rotary_valve.set_valve_position(2)
        self.move_reagent(self.V_to_V4, self.fast_speed, 1, self.empty_speed, 3)

        log.info("%s\t%i\t--> Prime position 3 with %i ul pre-loaded fluid" % \
            (self.cycle_name, self.flowcell, self.V_to_V4))
        # switch communication to ten port rotary valve V3
        self.mux.set_to_rotary_valve3()
        # switch rotary valve V3 to port 9
        self.rotary_valve.set_valve_position(9)
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 3
        self.rotary_valve.set_valve_position(3)
        self.move_reagent(self.V_to_V4, self.fast_speed, 1, self.empty_speed, 3)

        log.info("%s\t%i\t--> Draw %i ul guadinine up to V4-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.guadinine_to_V4))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 4
        self.rotary_valve.set_valve_position(4)
        self.move_reagent(5 * self.guadinine_to_V4, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT draw guadinine up to V4-4

        log.info("%s\t%i\t--> Draw %i ul NaOH up to V4-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.NaOH_to_V4))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 6
        self.rotary_valve.set_valve_position(6)
        self.move_reagent(5 * self.NaOH_to_V4, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT draw NaOH up to V4-9

        log.info("%s\t%i\t--> Draw %i ul dH2O up to V4-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.dH2O_to_V4))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 7
        self.rotary_valve.set_valve_position(7)
        self.move_reagent(5 * self.dH2O_to_V4, self.fast_speed, 1,\
             self.empty_speed, 3) #RCT draw dH2O up to V4-7

        log.info("%s\t%i\t--> Draw %i ul Wash up to V4-COM" % \
            (self.cycle_name, self.flowcell, 5 * self.wash1_to_V))
        # switch communication to ten port rotary valve V4
        self.mux.set_to_rotary_valve4()
        # switch rotary valve V4 to port 9
        self.rotary_valve.set_valve_position(9)
        self.move_reagent(5 * self.wash1_to_V, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT draw "Wash 1" up to V4-9

    """
    Prime ligase chamber
    """

    def prime_ligase(self):
        """
        Primes ligase and ligation buffer chambers in reagent cooling block.
        """
        self.ligase_prime_volume = self.ligase_to_V5 + self.V5_to_T
        log.info("%s\t%i\t--> Prime ligase chamber: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))
        log.info( ("%s\t%i\t--> Prime ligase chamber with %i ul " + \
                    "pre-loaded fluid") % (self.cycle_name, self.flowcell, \
                        self.ligase_chamber_volume))
        #RCT switch 2-way discrete valve V5 to NO (ligase)
        self.mux.discrete_valve5_open()
        self.move_reagent(self.ligase_prime_volume, self.fast_speed, 1, \
            self.empty_speed, 3) #RCT prime ligase chamber
        #RCT switch 2-way discrete valve V5 to NC (ligase)
        self.mux.discrete_valve5_close()

    """
    Reagent block priming
    """

    def prime_reagent_block(self, doV4):
        """
        Primes all reagent block chambers with 'Wash 1'.
        """

        log.info("%s\t%i\t--> Prime reagent block chambers: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))

        self.syringe_pump_init()  # initialize syringe pump
        self.prime_rotary_valve1()
        # prime reagent block chambers in ten port rotary valve V2
        self.prime_rotary_valve2()
        # prime reagent block chambers in ten port rotary valve V3
        self.prime_rotary_valve3()
        if(doV4 == 1):
            # prime reagent block chambers in ten port rotary valve V4
            self.prime_rotary_valve4()


    """
    Priming flowcells
    """

    def prime_flowcells(self):
        log.info("%s\t%i\t--> Prime both flowcells: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))
        self.flowcell=0
        self.flush_flowcell(9);
        self.flowcell=1
        self.flush_flowcell(9);
        "Primes both flowcells with 'Wash 1' as initialization step."



    """
    Fluidic sub-system priming
    """

    def prime_fluidics_system(self):
        """
        Primes all fluid lines, flowcells and reagent block chambers with
        'Wash 1'.
        Assume that all reagent block chambers and bottles are filled with
        'Wash 1'.
        """

        log.info("%s\t%i\t--> Prime fluidics system: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))
        self.prime_flowcells()      #RCT prime both flowcells with "Wash"
        self.prime_reagent_block()  #RCT prime reagent block chambers with "Wash"
        self.prime_flowcells()      #RCT prime both flowcells with "Wash"

    """
    INITIALIZATION FUNCTIONS
    """

    """
    Temperature control 1-2 initialization
    """

    def temperature_control_init(self):
        """
        Set temperature controller 1-2 to OFF state to avoid any temperature
        control operation left-over from a possible previous process.
        """

        log.info("%s\t%i\t--> Initialize temperature controller 1-2: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))

        # Switch communication to temperature controller 1
        self.mux.set_to_temperature_control1()
        # set flowcell temperature to room temperature
        self.temperature_control.set_temperature(self.room_temp)
        # wait until steady-state temperature is reached
        self.wait_for_SS(self.room_temp, self.room_temp, self.temp_tolerance)
         # turn external temperature controller OFF
        self.temperature_control.set_control_off()

        # Switch communication to temperature controller 2.
        self.mux.set_to_temperature_control2()
        # set flowcell temperature to room temperature
        self.temperature_control.set_temperature(self.room_temp)
        # wait until steady-state temperature is reached
        self.wait_for_SS(self.room_temp, self.room_temp, self.temp_tolerance)
        # turn external temperature controller OFF
        self.temperature_control.set_control_off()

    """
    Reagent block initialization
    """

    def reagent_block_init(self):
        """
        Initialize reagent block temperature and prime wells.
        """

        log.info("%s\t%i\t--> Initialize reagent block cooler: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))
        # set communication to reagent block cooler
        self.mux.set_to_reagent_block_cooler()
        # set reagent block temperature
        self.temperature_control.set_temperature(self.stage_temp)


    """
    Syringe pump initialization
    """

    def syringe_pump_init(self):
        """
        Initializes syringe pump by moving it to zero position
        and setting speed to 20.
        """

        log.info("%s\t%i\t--> Initialize syringe pump: [%s]" % \
            (self.cycle_name, self.flowcell, self.state))

        self.mux.set_to_syringe_pump()  # set communication to syringe pump
        self.syringe_pump.initialize_syringe()  # initialize syringe pump
        self.reagent_block_init()

    """
    Biochemistry initialization
    """

    def init(self):
        """
        Initialize biochemistry sub-system.
        """

        log.info("%s\t%i\t--> Initialize biochemistry sub-system: [%s]" % \
        (self.cycle_name, self.flowcell, self.state))

        log.info("Initialize temperature control")
        # initialize temperature controller 1/2
        self.temperature_control_init()

        log.info("Initialize syringe pump")
        # initialize syringe pump
        self.syringe_pump_init()

        log.info("Initialize reagent block")
        # set reagent block to constant temperature, 4 Celsius degrees
        # AND PRIME VALVES
        self.reagent_block_init()

    """
    BIOCHEMISTRY FUNCTIONS
    """


    """
    Strip_chem sub
    """

    """
    THIS FUNCTION ASSUMES ISOPROPANOL HAS BEEN LOADED INTO PORT 8 OF
    WHATEVER VALVE THE NONAMER MIX IS BEING DRAWN FROM (AS SPECIFIED
    BY GLOBAL VARIABLE self.port_scheme[self.cycle][2], WHICH COMES
    FROM config.txt AND THE VALUE ASSIGNED TO self.cycle WHEN THE
    biochem() OBJECT IS INSTANTIATED
    """

    def strip_chem(self, isoprop_valve, isoprop_port):
        """
        Performs chemical stripping protocol for polony sequencing.
        Does the following:

        - flush flowcell with dH2O
        - flush flowcell with guanidine HCl and incubate
        - flush flowcell with dH2O
        - flush flowcell with NaOH and incubate
        - flush flowcell with dH2O
        - flush flowcell with isopropanol
        - flush flowcell with 'Wash 1'
        """

        commands.getstatusoutput('mplayer -ao alsa speech/strip_chem.wav')

        t0 = time.time()  # get current time
        self.state = 'strip_chem' # update function state of biochemistry object

        log.info("%s\t%i\t--> In %s subroutine" % (self.cycle_name, \
            self.flowcell, self.state))

        if self.flowcell == 0:
            from_port = 1 #RCT set syringe pump port variable to 1
        else:
            from_port = 2 #RCT set syringe pump port variable to 2



        # FIRST, DRAW dH2O INTO FLOWCELL
        log.info("%s\t%i\t--> Draw %i ul dH2O into system" % \
            (self.cycle_name, self.flowcell, self.dH2O_volume))
        #RCT switch communication to ten port rotary valve V4 to select dH2O bottle
        self.mux.set_to_rotary_valve4()
        #RCT switch rotary valve V4 to port 7 (dH2O bottle)
        self.rotary_valve.set_valve_position(7)
        self.move_reagent(self.dH2O_volume, self.pull_speed, from_port, \
                self.empty_speed, 3) #RCT draw dH2O into system

        # NOW, MOVE GUANIDINE IN
        log.info("%s\t%i\t--> Draw %i ul guadinine into system" % \
            (self.cycle_name, self.flowcell, self.guadinine_volume))
        #guanidine (in port 4) is followed by dH2O (in port 7)
        self.draw_into_flowcell(7, 'V4', 4, self.guadinine_volume)
        # incubate reagent for guanidine_time minutes
        self.incubate_reagent(self.guadinine_time)

        #move 500ul isoprop into flowcell (port isoprop_port),
        # followed by Water 1 (port 9)
        self.draw_into_flowcell_bufferchase(9, 'V4', 5, 200,7,500)
        self.draw_into_flowcell(7, 'V4', 7, 1000) #one water wash.

        # NOW, MOVE NaOH INTO FLOWCELL
        log.info("%s\t%i\t--> Draw %i ul NaOH into system" % \
            (self.cycle_name, self.flowcell, self.NaOH_volume))
        #NaOH (in port 6) is followed by dH20 (in port 7)
        self.draw_into_flowcell(7, 'V4', 6, self.NaOH_volume)
        # incubate reagent for NaOH_time minutes
        self.incubate_reagent(self.NaOH_time)


        log.info("Flush flowcell w/ dH2O")
        self.flush_flowcell(7)  # run a flush routine with dH2O (port 7)


        # DO AN ISOPROPANOL FLUSH TO REMOVE ANY BUBBLES THAT HAVE ACCUMULATED
        # move 500ul isoprop into flowcell (port isoprop_port), followed by
        # Water 1 (port 9)
        self.draw_into_flowcell_bufferchase(9, 'V4', 5, 200,7,500)
        # move 500ul wash into flowcell (port isoprop_port), followed by
        # Wash 1 (port 9)
        self.draw_into_flowcell(9, 'V4', 9, 1000)

#        log.info("Flush flowcell w/ Wash 1")
# JSE commented out.  Too many washes slowing down the cycle time.
# We are doing a wash at the beginning of the hyb.
# should uncomment this for running without the hyb step.
#        self.flush_flowcell(9)


        delta = (time.time() - t0) / 60  # calculate elapsed time for stripping

        log.warn( ("%s\t%i\t--> Finished checmical strip - duration: " + \
            "%0.2f minutes\n") % (self.cycle_name, self.flowcell, delta))




    """
    Hyb sub
    """

    def hyb(self, primer_valve, primer_port):
        """
        Runs primer hybridization protocol for polony sequencing.
        Does the following:

        - incubate primer at 'hyb_temp1' for 'hyb_time1' minutes
        - incubate primer at 'hyb_temp2' for 'hyb_time2' minutes
        - flush flowcell with 'Wash 1'

        """

        commands.getstatusoutput('mplayer -ao alsa speech/hyb.wav')

        t0 = time.time()  # get current time
        self.state = 'hyb' # update function state of biochemistry object

        log.info("%s\t%i\t--> In %s subroutine" % (self.cycle_name, \
            self.flowcell, self.state))


        if self.flowcell == 0:
            from_port = 1 #RCT set syringe pump port variable to 2
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            from_port = 2 #RCT set syringe pump port variable to 3
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        # set flowcell temperature to hyb_set_temp1 C
        self.temperature_control.set_temperature(self.hyb_set_temp1)
        # wait until steady-state temperature is reached
        # self.wait_for_SS(self.hyb_set_temp1, self.hyb_poll_temp1, \
        #    self.temp_tolerance)
        #flushflow cell with Wash 1 (port 9)
        self.draw_into_flowcell(9, 'V4', 9, 1500)



        log.info( ("%s\t%i\t--> Draw %i ul anchor primer  into system from " + \
            "rotary valve") % (self.cycle_name, self.flowcell, \
            self.primer_volume))



        log.info("Move primer into flowcell from valve %s, port %d (%d ul)" % \
            (primer_valve, primer_port, self.primer_volume))
        #RCT push anchor primer into flowcell with Wash
        self.draw_into_flowcell(9, primer_valve, primer_port, \
            self.primer_volume) 

        log.info("Incubate primer for time1 at temp1")
        # incubate reagent for hyb_time1 min
        self.incubate_reagent(self.hyb_time1)

        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Set temperature to hyb_set_temp2")
        # set flowcell temperature to hyb_set_temp2 C
        self.temperature_control.set_temperature(self.hyb_set_temp2)
        # wait until steady-state temperature is reached
        # self.wait_for_SS(self.hyb_set_temp2, self.hyb_poll_temp2, \
        #    self.temp_tolerance)

        log.info("Incubate primer for time2 at temp2")
        # incubate reagent for hyb_time2 min
        self.incubate_reagent(self.hyb_time2)

        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Incubation finished; turn temperature control off")
        # turn external temperature controller OFF
        self.temperature_control.set_control_off()

        log.info("Flush flowcell with Wash 1")
        # flush entire flowcell w/ Wash
        self.flush_flowcell(9)
        #RCT set flowcell temperature to room temperature
        self.set_to_RT()

        # calculate elapsed time for primer hybridization
        delta = (time.time() - t0) / 60

        log.warn(("%s\t%i\t--> Finished primer hybridization - duration: " + \
            "%0.2f minutes\n") % (self.cycle_name, self.flowcell, delta))

    """
    Lig_stepup_peg sub
    """

    # THIS FUNCTION ASSUMES THAT LIGATION BUFFER HAS BEEN LOADED INTO PORT 8
    # OF WHATEVER VALVE THE NONAMER MIX IS BEING DRAWN FROM
    #
    def lig_stepup_peg(self, nonamer_valve, nonamer_port):
        """
        Runs stepup peg ligation reaction protocol for polony sequencing.
        Does the following:

        - prime flowcell with Quick ligase buffer
        - incubate quick ligation mix at the following steps:
        - flush flowcell with 'Wash 1'

        Reagent requirements:
        """


        commands.getstatusoutput('mplayer -ao alsa speech/lig_stepup_peg.wav')

        t0 = time.time()  # get current time
        # update function state of biochemistry object
        self.state = 'lig_stepup_peg'

        log.info("%s\t%i\t--> In %s subroutine" % \
        (self.cycle_name, self.flowcell, self.state))

        if self.flowcell == 0:
            from_port = 1 #RCT set syringe pump port variable to 2
        else:
            from_port = 2 #RCT set syringe pump port variable to 3


        # SET TO REACTION TEMPERATURE 1
        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Set temperature to lig_set_step1: %d" %(self.lig_set_step1))
        self.temperature_control.set_temperature(self.lig_set_step1)

        # MOVE REAGENTS INTO FLOWCELL (NONAMER MIX, PRECEDED BY LIGATION BUFFER
        # AND FOLLOWED BY WASH)
        log.info( ("Move nonamer mix (%s,%d), preceded by ligation buffer " + \
                "(%s,%d), into flowcell") % \
                (nonamer_valve, nonamer_port, nonamer_valve, 8))
        #RCT pull up ligation buffer
        self.draw_reagent(nonamer_valve, 8, self.buffer_volume)

        # DON'T START THE TIMER UNTIL WE'VE REACHED THE TEMPERATURE
        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        # wait until steady-state temperature is reached
        # self.wait_for_SS(self.lig_set_step1, self.lig_poll_step1, \
        # self.temp_tolerance)
        self.draw_into_flowcell_bufferchase(9, nonamer_valve, nonamer_port, \
            self.nonamer_volume,8,300)  #RCT push

        # INCUBATE
        log.info("Incubate ligation step 1")
        self.incubate_reagent(self.lig_time1)


        # SET TO REACTION TEMPERATURE 2
        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Set temperature to lig_set_step2: %d" %(self.lig_set_step2))
        self.temperature_control.set_temperature(self.lig_set_step2)
        # self.wait_for_SS(self.lig_set_step2, self.lig_poll_step2, \
        # self.temp_tolerance)  # wait until steady-state temperature is reached

        # INCUBATE
        log.info("Incubate ligation step 2")
        self.incubate_reagent(self.lig_time2)


        # SET TO REACTION TEMPERATURE 3
        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Set temperature to lig_set_step3: %d" %(self.lig_set_step3))
        self.temperature_control.set_temperature(self.lig_set_step3)
        # self.wait_for_SS(self.lig_set_step3, self.lig_poll_step3, \
        # self.temp_tolerance)  # wait until steady-state temperature is reached

        # INCUBATE
        log.info("Incubate ligation step 3")
        self.incubate_reagent(self.lig_time3)


        # SET TO REACTION TEMPERATURE 4
        if self.flowcell == 0:
            # set to temperature controller 1
            self.mux.set_to_temperature_control1()
        else:
            # set to temperature controller 2
            self.mux.set_to_temperature_control2()

        log.info("Set temperature to lig_set_step4: %d" %(self.lig_set_step4))
        self.temperature_control.set_temperature(self.lig_set_step4)
        # wait until steady-state temperature is reached
        # self.wait_for_SS(self.lig_set_step4, self.lig_poll_step4, \
        # self.temp_tolerance)

        # INCUBATE
        log.info("Incubate ligation step 4")
        self.incubate_reagent(self.lig_time4)

        # SET TEMPERATURE CONTROL TO ROOM TEMP
        self.set_to_RT()  #RCT set flowcell temperature to room temperature

        # FLUSH FLOWCELL
        self.flush_flowcell(9)  # flush entire flowcell with Wash
        self.flush_flowcell(9)  # flush entire flowcell with Wash;
                                # ADDED BY GP 10-07-2008

        # calculate elapsed time for stepup peg ligation
        delta = (time.time() - t0) / 60

        log.warn( ("%s\t%i\t--> Finished step-up peg ligation - duration: " + \
                 "%0.2f minutes\n") % (self.cycle_name, self.flowcell, delta))


    """
    SEQUENCING ALGORTIHMS
    """


    """
    Run sub
    """

    def run(self):
        """
        Runs polony sequencing cycle(s) based on cycle-name and flowcell-number
        list already contained in biochemistry object.
        """

        time.sleep(1)
        self.state = 'running'  # update function state of biochemistry object

        """
        Flowcell preparation
        """

        # if white light image cycle on flowcell 0
        if self.cycle[0:2] == 'WL' and self.flowcell == 0:
            self.init()                    # do only once at beginning
            #self.exo_start()
            self.prime_reagent_block()
            log.info( ("%s\t%i\t--> Device initialization and Exonuclease " + \
                "I digestion is done: [%s]\n") % \
                (self.cycle_name, self.flowcell, self.state))

        # if white light image cycle on flowcell 1
        elif self.cycle[0:2] == 'WL' and self.flowcell == 1:
            self.init()
            #self.exo_start()
            self.prime_reagent_block()
            log.info("%s\t%i\t--> Exonuclease I digestion is done: [%s]\n" % \
                (self.cycle_name, self.flowcell, self.state))
        #RCT elif self.cycle == 'EXP':
        elif 'EXP' in self.cycle: #RCT added for SBS
            self.runCustomExperiment()
        elif self.cycle == 'ILMD':
            self.runILMNDeblock(self.flowcell)
        elif self.cycle == 'ILMC':
            self.runILMNCycle(self.flowcell)
        else:
            self.cycle_ligation()  # perform query cycle on selected flowcell


    """
    Cycle_ligation sub
    """

    def cycle_ligation(self):
        """
        Performs a cycle of polony sequencing biochemistry consisting of:

        - chemical stripping
        - primer hybridization
        - query nonamer ligation (stepup temperature, peg)

        Reagent requirements:

        - see 'cycle_list' valve-port map in configuration file
        """

        t0 = time.time()  # get current time
        self.state = 'cycle_ligation' # update function state of biochemistry object

        log.info("%s %s\t%i\t--> In %s subroutine" % \
            (self.cycle_name, self.cycle, self.flowcell, self.state))
        self.syringe_pump_init()  # initialize syringe pump
        isoprop_valve = 'V4'
        isoprop_port = 5
        # perform polony sequencing biochemistry
        self.strip_chem(isoprop_valve, isoprop_port)

        # get anchor primer rotary valve from configuration schematics
        primer_valve = self.port_scheme[self.cycle][0]
        # get anchor primer port on rotary valve from configuration schematics
        primer_port = self.port_scheme[self.cycle][1]
        self.hyb(primer_valve, primer_port)

        # get nonamer rotary valve from configuration schematics
        nonamer_valve = self.port_scheme[self.cycle][2]
        # get nonamer port on rotary valve from configuration schematics
        nonamer_port = self.port_scheme[self.cycle][3]
        self.lig_stepup_peg(nonamer_valve, nonamer_port)

        delta = (time.time() - t0) / 60  # calculate elapsed time for polony cycle

        log.warn("%s\t%i\t--> Finished cycle ligation - duration: %0.2f minutes\n" % (self.cycle_name, self.flowcell, delta))



##############################################################################
##############################################################################
##################      BEGIN 'CUSTOMIZABLE' CODE              ###############

    """
    Example of a function to perform an 'experimental' biochemistry cycle on the
     Polonator.
    This can be modified to suit the particular biochemical steps you need to
    perform.
    strip_chem(), hyb(), and lig_stepup_peg() are special cases of biochemistry
    reactions that are used by the standard Polonator SBL biochemistry.
    React() allows you to run arbitrary reactions.
    By chaining these together, as in the example below, you can do pretty much
    any ligase- or polymerase- based biochemistry.

    added by Greg Porreca, Oct-16-2008
    modified by Richard Terry Jun-6-2009
    """
    def runCustomExperiment(self):
        """
        hard-coded reagent locations and reaction parameters for this experiment

        DO NOT use port 8 for anything, as lig_stepup_peg assumes that port 8 of
        the valve which contains the nonamer it's using has been filled with
        ligation buffer
        DO NOT use V4 port 5 for anything, as strip_chem assumes it is filled with
        isopropanol for its use
        """
        hyb_valve = 'V2'
        hyb_port = 1
        lig1_valve = 'V1'
        lig1_port = 1
        lig2_valve = 'V1'
        lig2_port = 2
        react_valve = 'V3' # location (valve) of the PNK reaction mix
        react_port = 1     # location (port) of the PNK reaction mix
        react_temp = 35    # un-calibrated PNK reaction temperature
                           # (i.e. temp at the glass will vary slightly)
        react_time = 30    # PNK reaction time in minutes


        # DON'T TOUCH THIS!
        # It is necessary to assign this global variable to
        # this value in order for strip_chem to work
        #RCT self.cycle = "AM1"

        # always initialize the syringe at the start of a cycle
        self.syringe_pump_init()

        # strip previous signal, and hyb sequencing primer
        # temperatures and times for the hyb reaction are pulled from config.txt
        #RCT self.strip_chem()
        #RCT self.hyb(hyb_valve, hyb_port)

        """
        ligate hexamer spacer onto anchor
        lig_stepup_peg assumes port 8 on the lig_valve has been filled w/ buffer
        temperatures and times for the ligation reaction are pulled from
        config.txt
        """
        # RCT self.lig_stepup_peg(lig1_valve, lig1_port)

        # 5'-phosphorylate hexamer so it can be ligated to
        # for this reaction, we will not 'prime' the flowcell with a buffer
        #RCTself.react(react_valve, react_port, react_temp, react_time)
        self.react('V1', 2, 55, 20, 0, 0)
        self.react('V1', 3, 55, 20, 0, 0)
        self.react('V1', 1, 55, 5, 0, 0)

        # now perform the query nonamer ligation
        #RCT self.lig_stepup_peg(lig2_valve, lig2_port)



    """
    Call this function to perform any arbitrary reaction in the flowcell.
     Arguments are the following:
    reagent_valve:  STRING specifying the valve where the reagent (and
                    optionally buffer) has been loaded; valid values are
                    "V1", "V2", and "V3"
    reagent_port:   INTEGER specifying the port on the reagent valve where the
                    reagent has been loaded; valid values are [1..8]
    temp:           INTEGER specifying the temperature the reaction should be
                    run at;
                    the flowcell will be brought to temperature temp before the
                    reagent is moved in;
                    valid values are [18..60]
    react_time:     INTEGER specifying the time in minutes to allow the reaction
                     to incubate
    buffer_before:  INTEGER flag specifying whether buffer should be flowed
                     before the reagent
    buffer_after:   INTEGER flag specifying whether buffer should be flowed
                     after the reagent
    buffer_port:    OPTIONAL, INTEGER specifying the port on the reagent valve
                     (NOTE if a buffer is used, it MUST be loaded on the same
                      valve as the reagent) where the buffer has been loaded;
                       valid values are [1..8]
    buffer_volume:  OPTIONAL, INTEGER specifying the volume of buffer to pull
                     before/aftr the reagent,
                    if a buffer is used.  If a buffer is used, and buffer_volume
                     is not specified,
                    react() will use reagent_volume for buffer_volume. If a
                     buffer is not used, buffer_volume need not be specified.
                     If a buffer is not used and buffer_volume is specified,
                     it will be ignored.

    ASSUMES REACTION WILL BE PERFORMED ON FLOWCELL 0

    added by Greg Porreca, Oct-16-2008
    modified by Richard Terry Jun-6-2009
    """
    def react(self, reagent_valve, \
                reagent_port, temp, \
                react_time, buffer_before, \
                buffer_after, buffer_port=None, \
                buffer_volume=0):

        timeout = 120 # timeout in seconds when polling temperature
        tolerance = 2 # tolerance in degrees C when polling temperature
        # port for reagent used to flush flowcell at conclusion of reaction
        flush_port = 9
        # volume, in microliters, of reagent used in reaction
        reagent_volume = 278
        flowcellnum = 0

        self.flowcell = flowcellnum
        if self.flowcell == 0:
            from_port = 1
        else:
            from_port = 2

        # have buffer volume been specified?  if not, deal with this
        if buffer_volume == 0:
            buffer_volume = reagent_volume

        # set the flowcell to the reaction temperature
        self.setTemp(flowcellnum, temp)
        self.flush_flowcell(flush_port)

        # are we moving a buffer ahead of the reagent in the lines?
        if buffer_before == 1:
            if buffer_port is not None:
                self.draw_reagent(reagent_valve, buffer_port, buffer_volume)

        # wait for the flowcell to reach the reaction temperature
        #self.waitForTemp(temp, timeout, flowcellnum, tolerance)t)

        # move the reagent into the flowcell; if there was a buffer first, this
        # will automatically be pushed through
        # different functions must be used if we're chasing w/ buffer
        if buffer_after == 1:
            if buffer_port is not None:
                self.draw_into_flowcell_bufferchase(9, reagent_valve, \
                    reagent_port, reagent_volume, buffer_port, buffer_volume)
        else:
            self.draw_into_flowcell(9, reagent_valve, \
                reagent_port, reagent_volume)

        # allow reaction to incubate for react_time minutes
        self.incubate_reagent(react_time)

        # set flowcell temperature back to room temperature, but don't wait
        # for it to get there
        self.setTemp(flowcellnum, self.room_temp)

        # flush reaction mixture out of flowcell
        self.flush_flowcell(flush_port)
        self.flush_flowcell(flush_port)




##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################
##############################################################################
    """
    FUNCTIONS BELOW ARE INDEPENDENT OF LEGACY CODE IN FILE BIOCHEM.PY
    THEY CAN BE USED TO BUILD STANDALONE METHODS FOR BIOCHEMISTRY WHICH
    DO NOT RELY ON ANY FUNCTIONS FOUND ABOVE THIS POINT IN THE FILE

    GP 01-23-2009

    """
    """
    Call this function to 'rinse' the flowcell; generally speaking,
    it pulls a large volume of liquid
    over the flowcell from rinse_port on rinse_valve.  It first pulls fast_vol
    microliters at fast_speed
    (generally pulling a vacuum, so volume is not accurate), then pulls
    slow_vol microliters at slow_speed

    added by Greg Porreca, Jan-23-2009
    modified by Richard Terry Jun-6-2009
    """
    # def rinse(self, flowcellnum, rinse_valve='V4', \
    #            rinse_port=9, fast_vol=2000, slow_vol=2000, \
    #           fast_speed=30, slow_speed=30):
    def rinse(self, flowcellnum, rinse_valve='V4', \
                    rinse_port=9, fast_vol=4000, \
                    slow_vol=2000, fast_speed=25, slow_speed=29):
        self.empty_speed = 0
        self.rotary_valve.usePort(rinse_valve, rinse_port)
        self.syringe_pump.draw(fast_vol, flowcellnum, fast_speed,\
             self.empty_speed)
        self.syringe_pump.draw(slow_vol, flowcellnum, slow_speed,\
             self.empty_speed)

    """
    Call this function to 'pump' reagent into the system from a specific port on
    a specific valve
    Note this just pulls the reagent into the system, and doesn't know anything
    about where the reagent
    ends up (i.e in the tubing, in the flowcell, etc).  One should understand
    the path length between
    reagent block and flowcell, and the flowcell volume, before using this

    added by Greg Porreca, Jan-23-2009
    """
    def pumpReagent(self, flowcellnum, volume, valve, port, speed=27):
        self.empty_speed = 0
        self.rotary_valve.usePort(valve, port)
        self.syringe_pump.draw(volume, flowcellnum, speed, self.empty_speed)

    """
    Call this function after a call to temperature_control.set_temperature()
    (which changes the temperature
    controller's setpoint).  This will block until either: 1) the difference
    between the temperature read
    and setpoint is <= tolerance, or 2) the elapsed time is >= timeout seconds

    added by Greg Porreca, Oct-16-2008
    """
    def waitForTemp(self, setpoint, timeout, flowcellnum, tolerance=None):

        # for unspecified tolerance, assume 1C
        if tolerance is None:
            tolerance = 1

        log.debug( (" waiting maximum of %d seconds until temperature is " + \
            "within %d C of %d C setpoint") %(timeout, tolerance, setpoint))

        # set the mux
        self.muxToFC(flowcellnum)

        # time we started polling
        t0 = time.time()

        while( (math.fabs(self.temperature_control.get_temperature() - \
                setpoint) > tolerance) and \
                (math.fabs(time.time() - t0) < timeout)):
            sys.stderr.write( ("\tcurrent temperature: %03d C, elapsed time " + \
                "%04d seconds\r") % (self.temperature_control.get_temperature(),\
                     math.fabs(time.time()-t0)))
            sys.stderr.flush()
            time.sleep(1)
        sys.stderr.write("\n")


    """
    Call this to set the temperature for flowcell flowcellnum to temperature
    temp

    added by Greg Porreca, Oct-17-2008
    """
    def setTemp(self, flowcellnum, temp):
        self.muxToFC(flowcellnum)
        self.temperature_control.set_temperature(temp)


    """
    Call this to set the mux to a flowcell temperature controller; used by
    setTemp and waitForTemp

    added by Greg Porreca, Oct-17-2008
    modified [GP] 01-23-2009 for mux rewrite
    """
    def muxToFC(self, flowcellnum):
        self.mux.setToDevice('TCFC%d' %(flowcellnum))


    def primeReagentBlock(self, flowcellnum, doV4=0):
        prime_speed = 30
        prime_volume = (0, 400, 400, 400, 1000) # volumes for V1, V2, V3, V4

        if(doV4 == 1):
            maxValveRange = 5 # valves 1-4
        else:
            maxValveRange = 4 # valves 1-3

        for valve in range(1,maxValveRange):
            # do ports 1-9 in reverse order; port 10 should be plugged,
            # so don't prime this
            for port in range(9,0,-1):
                log.info('Prime valve %d, port %s with %d ul' % \
                    (valve, port, prime_volume[valve]))
                self.rotary_valve.usePort('V' + str(valve), port)
                # don't prime plugged ports on V4
                if((valve == 4) and (port == 8)):
                    log.info('Skip current position because it is plugged')
                else:
                    self.syringe_pump.draw(prime_volume[valve], \
                        flowcellnum, prime_speed, 0)


    """
    Perform a cycle of Illumina deblock chemistry;
    reagent location are hardcoded on VALVE 2:
    cleavage buffer:      port 1  1000 ul
    cleavage mix:         port 2  1000 ul
    incorporation buffer: port 3  600 ul
    high-salt buffer:     port 4  600 ul
    scan buffer:          port 5  800 ul

    added by Greg Porreca, Jan-23-2009
    """
    def runILMNDeblock(self, flowcellnum, deblock_valve='V2'):
        log.info("Executing runILMNDeblock %d %s" %(flowcellnum, deblock_valve))

        # all deblock reagents should be kept on one valve, and this should
        # not be the same valve as the 'cycle' or SBS incorporation reagents
        CB_port = 8
        CM_port = 1
        IB_port = 2
        HS_port = 3
        SB_port = 4

        self.ilmnDeblock(flowcellnum, deblock_valve, CB_port, deblock_valve, \
            CM_port, deblock_valve, IB_port, deblock_valve, HS_port, \
            deblock_valve, SB_port)
        log.info("Finished executing runILMNDeblock()")


    """
    Performs a cycle of Illumina 'cycle' or SBS-incorporation chemistry;
    reagent locations are hardcoded on VALVE 1:
    incorporation buffer: port 1  1600 ul
    incorporation mix:    port 2  1000 ul
    high-salt buffer:     port 3  600 ul
    scan buffer:          port 4  800 ul

    added by Greg Porreca, Jan-23-2009
    """
    def runILMNCycle(self, flowcellnum, cycle_valve='V1'):
        log.info("Executing runILMNCycle %d %s" %(flowcellnum, cycle_valve))

        # all cycle reagents should be kept on one valve, and this should
        # not be the same valve as the deblock reagents
        IB_port = 8
        IM_port = 1
        HS_port = 2
        SB_port = 3

        self.ilmnCycle(flowcellnum, cycle_valve, IB_port, cycle_valve, \
            IM_port, cycle_valve, HS_port, cycle_valve, SB_port)
        log.info("Finshed executing runILMNCycle()")


    """
    Performs a sequencing primer hybridization for Illumina chemistry
    finishes by flushing with incorporation mix for first incorporation
    reagent locations are hardcoded on VALVE 1:
    hybridization mix:    port 5  1000 ul
    incorporation buffer: port 1  1000 ul

    added by Greg Porreca, Jan-26-2009
    """
    def runILMNHyb(self, flowcellnum, cycle_valve='V1'):
        log.info("Executing runILMNHyb %d %s" %(flowcellnum, cycle_valve))

        # these reagents should be kept on the 'cycle', or incorporation, valve
        HYB_port = 4
        WASH_port = 9

        self.ilmnHyb(flowcellnum, cycle_valve, HYB_port, cycle_valve, WASH_port)
        log.info("Finished executing runILMNHyb()")


    """
    Perform Illumina's standard 'deblock' protocol using default reaction
    conditions

    added by Greg Porreca, Jan-23-2009
    """
    def ilmnDeblock(self, flowcellnum, CB_valve, CB_port, CM_valve, CM_port, \
        IB_valve, IB_port, HS_valve, HS_port, SB_valve, SB_port):
        log.info(("Executing ilmnDeblock %d CB<%s,%d> CM<%s,%d> IB<%s,%d> " + \
            "HS<%s,%d> SB<%s,%d>") %(   flowcellnum, \
                                        CB_valve, CB_port, \
                                        CM_valve, CM_port, \
                                        IB_valve, IB_port, \
                                        HS_valve, HS_port, \
                                        SB_valve, SB_port))

        # set temperature to 52C and allow to ramp during next fluid move
        set_temp=52
        log.info(" set temperature to %dC" %(set_temp))
        self.setTemp(flowcellnum, set_temp)

        # pump 1 ml of cleavage buffer (CB)
        volume = 1000
        log.info(" pump %d ul of cleavage buffer from <%s,%d>" % \
            (volume, CB_valve, CB_port))
        self.pumpReagent(flowcellnum, volume, CB_valve, CB_port)

        # now wait for temp to reach setpoint
        timeout=300
        log.info(" wait up to %d seconds for temp to reach %dC setpoint" % \
            (timeout, set_temp))
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # pump 800ul of cleavage mix (CM), then wait 4 minutes
        wait_time=240
        volume=800
        log.info( (" pump %d ul of cleavage mix from <%s,%d> and incubate " + \
            "for %d seconds") % (volume, CM_valve, CM_port, wait_time))
        self.pumpReagent(flowcellnum, volume, CM_valve, CM_port)
        time.sleep(wait_time)

        # pump 200ul of cleavage mix (CM), then wait 4 minutes
        wait_time=240
        volume=200
        log.info( (" pump %d ul of cleavage mix from <%s,%d> and incubate " + \
            "for %d seconds") % (volume, CM_valve, CM_port, wait_time))
        self.pumpReagent(flowcellnum, volume, CM_valve, CM_port)
        time.sleep(wait_time)

        # pump 200ul of cleavage buffer (CB), then wait 4 minutes;
        # incubation is still with CM because of fluid line volume
        wait_time=240
        volume=200
        log.info((" pump %d ul of cleavage buffer from <%s,%d> and incubate" + \
            " for %d seconds (incubation is still with CM)") % \
                (volume, CB_valve, CB_port, wait_time))
        self.pumpReagent(flowcellnum, volume, CB_valve, CB_port)
        time.sleep(wait_time)

        # set temperature to 25C and allow to ramp during fluid moves
        set_temp=25
        log.info(" set temperature to %dC" %(set_temp))
        self.setTemp(flowcellnum, set_temp)

        # pump 600ul incorporation buffer (IB)
        volume = 600
        log.info(" pump %d ul of incorporation buffer from <%s,%d>" % \
            (volume, IB_valve, IB_port))
        self.pumpReagent(flowcellnum, volume, IB_valve, IB_port)

        # pump 600ul high salt (HS) buffer
        volume = 600
        log.info(" pump %d ul of high-salt buffer from <%s,%d>" % \
            (volume, HS_valve, HS_port))
        self.pumpReagent(flowcellnum, volume, HS_valve, HS_port)

        # rinse flowcell thoroughly with Wash 1
        num_rinses = 3
        log.info(" rinse %d times with Wash 1 from valve %s" % \
            (num_rinses, CM_valve))
        for i in range(1,num_rinses+1):
            log.info(" - perform rinse %d / %d" %(i, num_rinses))
            self.rinse(flowcellnum, CM_valve)

        # wait for temperature to reach setpoint if necessary
        timeout=300
        log.info(" wait up to %d seconds for temp to reach %dC setpoint" % \
            (timeout, set_temp))
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # pump 800ul scan buffer (SB)
        volume = 800
        log.info(" pump %d ul of scan buffer from <%s,%d>" % \
            (volume, SB_valve, SB_port))
        self.pumpReagent(flowcellnum, volume, SB_valve, SB_port)

        log.info("Finished executing ilmnDeblock()")


    # Perform Illumina's standard 'cycle', or SBS incorporation, protocol
    # using default reaction conditions
    #
    # added by Greg Porreca, Jan-23-2009
    #
    def ilmnCycle(self, flowcellnum, IB_valve, \
                        IB_port, IM_valve, IM_port, \
                        HS_valve, HS_port, \
                        SB_valve, SB_port):
        log.info(("Executing ilmnCycle %d IB<%s,%d> IM<%s,%d> HS<%s,%d> " + \
                    "SB<%s,%d>") % (flowcellnum, \
                                    IB_valve, IB_port, \
                                    IM_valve, IM_port, \
                                    HS_valve, HS_port, \
                                    SB_valve, SB_port))

        # set temperature to 52C and allow to ramp during next fluid move
        set_temp=52
        log.info(" set temperature to %dC" % (set_temp))
        self.setTemp(flowcellnum, set_temp)

        # pump 1 ml if incorporation buffer (IB)
        volume = 1000
        log.info(" pump %d ul of incorporation buffer from <%s,%d>" % \
            (volume, IB_valve, IB_port))
        self.pumpReagent(flowcellnum, volume, IB_valve, IB_port)

        # now wait for temp to reach setpoint
        timeout=300
        log.info(" wait up to %d seconds for temp to reach %dC setpoint" % \
            (timeout, set_temp))
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # pump 800ul incorporation mix (IM), wait for 4 minutes
        wait_time=240
        volume=800
        log.info((" pump %d ul of incorporation mix from <%s,%d> and " + \
            "incubate for %d seconds") %(volume, IM_valve, IM_port, wait_time))
        self.pumpReagent(flowcellnum, volume, IM_valve, IM_port)
        time.sleep(wait_time)

        # pump 200ul incorporation mix (IM), wait for 4 minutes
        wait_time=240
        volume=200
        log.info( (" pump %d ul of incorporation mix from <%s,%d> and " + \
                "incubate for %d seconds") % \
                (volume, IM_valve, IM_port, wait_time))
        self.pumpReagent(flowcellnum, volume, IM_valve, IM_port)
        time.sleep(wait_time)

        # pump 200ul incorporation buffer (IB),
        # wait for 4 minutes (this moves IM into flowcell)
        wait_time=240
        volume=200
        log.info((" pump %d ul of incorporation buffer from <%s,%d> and " + \
                "incubate for %d seconds (incubation is still with IM)") % \
                (volume, IB_valve, IB_port, wait_time))
        self.pumpReagent(flowcellnum, volume, IB_valve, IB_port)
        time.sleep(wait_time)

        # set temperature to 25C and allow to ramp during fluid moves
        set_temp=25
        log.info(" set temperature to %dC" %(set_temp))
        self.setTemp(flowcellnum, set_temp)

        # pump 600ul incorporation buffer (IB)
        volume = 600
        log.info(" pump %d ul of incorporation buffer from <%s,%d>" % \
            (volume, IB_valve, IB_port))
        self.pumpReagent(flowcellnum, volume, IB_valve, IB_port)

        # pump 600ul high-salt (HS) buffer
        volume = 600
        log.info(" pump %d ul of high-salt buffer from <%s,%d>" % \
            (volume, HS_valve, HS_port))
        self.pumpReagent(flowcellnum, volume, HS_valve, HS_port)

        # rinse flowcell thoroughly with Wash 1
        num_rinses = 3
        log.info(" rinse %d times with Wash 1 from valve %s" % \
            (num_rinses, IM_valve))
        for i in range(1,num_rinses+1):
            log.info(" - perform rinse %d / %d" %(i, num_rinses))
            self.rinse(flowcellnum, IM_valve)


        # wait for temperature to reach setpoint if necessary
        timeout=300
        log.info(" wait up to %d seconds for temp to reach %dC setpoint" % \
            (timeout, set_temp))
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # pump 800ul scan buffer (SB)
        volume = 800
        log.info(" pump %d ul of scan buffer from <%s,%d>" % \
            (volume, SB_valve, SB_port))
        self.pumpReagent(flowcellnum, volume, SB_valve, SB_port)

        log.info("Finished executing ilmnCycle()")


    # Perform primer hybridization before Illumina's standard 'cycle' protocol
    # using default reaction conditions
    #
    # added by Greg Porreca, Jan-26-2009
    #
    def ilmnHyb(self, flowcellnum, HYB_valve, HYB_port, WASH_valve, WASH_port):
        log.info("Executing ilmnHyb %d HYB<%s,%d> WASH<%s,%d>" %(flowcellnum,
                                       HYB_valve, HYB_port,
                                       WASH_valve, WASH_port))


        # set temperature to 50C, wait for up to 5 minutes
        set_temp=50
        timeout=300
        log.info(" set temperature to %dC, waiting for up to %d seconds" % \
            (set_temp, timeout))
        self.setTemp(flowcellnum, set_temp)
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # pump 800 ul of hyb solution
        volume = 800
        wait_time = 60
        log.info((" pump %d ul of hyb mix from <%s,%d>, then incubate for " + \
            "%d seconds") % (volume, HYB_valve, HYB_port, wait_time))
        self.pumpReagent(flowcellnum, volume, HYB_valve, HYB_port)
        time.sleep(wait_time)

        # pump 200 ul of hyb solution
        volume = 200
        wait_time = 60
        log.info(("pump %d ul of hyb mix from <%s,%d>, then incubate for %d" + \
                " seconds") %(volume, HYB_valve, HYB_port, wait_time))
        self.pumpReagent(flowcellnum, volume, HYB_valve, HYB_port)
        time.sleep(wait_time)

        # pump 200 ul of wash buffer
        volume = 200
        wait_time = 60
        log.info((" pump %d ul of wash buffer from <%s,%d> (primer still" + \
                " in flowcell), then incubate for %d seconds") % \
                    (volume, WASH_valve, WASH_port, wait_time))
        self.pumpReagent(flowcellnum, volume, WASH_valve, WASH_port)
        time.sleep(wait_time)

        # set temperature to 45C, wait for up to 5 minutes
        set_temp=45
        timeout=300
        log.info(" set temperature to %dC, waiting for up to %d seconds" % \
            (set_temp, timeout))
        self.setTemp(flowcellnum, set_temp)
        self.waitForTemp(set_temp, timeout, flowcellnum)

        # set temperature to 25C and continue
        set_temp=25
        log.info((" set temperature to %dC, and allow to ramp during next" + \
                    " step(s)") % (set_temp))
        self.setTemp(flowcellnum, set_temp)

        # rinse flowcell thoroughly with Wash 1
        num_rinses = 3
        log.info(" rinse %d times with Wash 1 from valve %s" % \
            (num_rinses, HYB_valve))
        for i in range(1,num_rinses+1):
            log.info(" - perform rinse %d / %d" %(i, num_rinses))
            self.rinse(flowcellnum, HYB_valve)


        # wait for temperature to reach setpoint before exiting
        log.info(" waiting for up to %d seconds for temperature to reach %d" % \
            (timeout, set_temp))
        self.waitForTemp(set_temp, timeout, flowcellnum)

        log.info("Finished executing ilmnHyb()")

