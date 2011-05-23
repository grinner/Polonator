"""
--------------------------------------------------------------------------------
 Author: Rich Terry.
 Date: February 12, 2008.
 Modified by: Mirko Palla
 Date: March 5, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab -
 Genetics Department, Harvard Medical School.

 Purpose: This program contains the complete code for class Rotary_valve,
 containing rotary valve communication subroutines in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
-------------------------------------------------------------------------------
"""
import time
import logging

log = logging.getLogger("r_valve")

class Rotary_valve:

    global serport;

    def __init__(self, config, serial_port, mux, logger=None):
        "Initialize Rheodyne rotary valve object with default parameters."

        self._baud_rate = int(config.get("communication","rotary_valve_baud"))
        self._read_length = int(config.get("communication","read_length"))
        self._sleep_time = float(config.get("communication","sleep_time"))

        self.serport = serial_port
        self.mux = mux
        self.state = 'rotary valve initialized'

        log.debug("---\t-\t-->     rotary valve object constructed")


    def usePort(self, valve_name, valve_port):
        log.debug("---\t-\t-->     use rotary valve %s position %d" % \
            (valve_name, valve_port))
        log.debug("---\t-\t-->     set valve %s to position %d" % \
            (valve_name, valve_port) )

        self.mux.setToDevice(valve_name)
        self.set_valve_position(valve_port)

        if(valve_name != 'V4'):
            log.debug("---\t-\t-->     set valve %s to position %d" % \
                ('V4', int(valve_name[1])))
            self.mux.setToDevice('V4')
            self.set_valve_position(int(valve_name[1]))
        # end if
    # end def

    """
    Rheodyne rotary valve
    """
    # FUNCTIONS
    """

    Performs low-level functional commands (e.g. set rotary valve position).
    Each command implemented here must know the command set of the hardware
    being controlled, but does not need to know how to communicate with the
    device (how to poll it, etc). Each functional command will block until
    execution is complete.

    """
    # BASIC SETTINGS

    def set_valve_position(self, valve_position):
        """
        Switch valve to given port on rotary valve, an integer.
        """


        valve_position = '0' + (str(hex(valve_position)[2:])).capitalize()
        valve_position_string = 'P' + valve_position + '\r'
        log.debug("---\t-\t-->     set rotary valve to position %s" % \
            valve_position_string)

        self.serport.set_baud(self._baud_rate)   # set baud rate of rotary valve
        self.serport.write_serial(valve_position_string)

        read_chars = '*'
        i = 1
        time.sleep(0.01)
        valve_position = valve_position + '\r'
        while (read_chars != valve_position):
            self.serport.write_serial('S\r')
            time.sleep(0.1)
            read_chars = self.serport.read_serial_r(3)
            i = i + 1

            if (i >= 10):
                i = 0
                self.serport.write_serial(valve_position_string)
                time.sleep(0.1)

        find_string = valve_position
        response_string_size = 3

        self.serport.parse_read_string('S\r', find_string, response_string_size)

