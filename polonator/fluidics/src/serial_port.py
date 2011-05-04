"""
--------------------------------------------------------------------------------
 Author: Richard Terry.
 Date: February 12, 2008.
 Modified by: Mirko Palla
 Date: March 5, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.
 
 Purpose: This program contains the complete code for class Serial_port,
 containing a set of serial port device communication subroutines in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file. 
------------------------------------------------------------------------------- 
"""

import time
import serial
import logging

log=logging.getLogger("ser_port")

class Serial_port:

    global ser

    def __init__(self, config, logger=None):
        "Initialize serial port object with default parameters."

        self.ser = serial.Serial( \
            port = config.get("communication","serial_port"),
            bytesize = serial.EIGHTBITS,
            parity = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout = float(config.get("communication","timeout")))

        self.device_bauds = { \
            int(config.get("communication",\
                "temperature_control_baud")) : "temperature controller", \
            int(config.get("communication", \
                "syringe_pump_baud")) : "syringe pump", \
            int(config.get("communication", \
                "rotary_valve_baud")) : "rotary valve" \
            }

        log.debug("---\t-\t--> SR: serial port object constructed")

    """
    SERIAL FUNCTIONS
    Serial command interface protocols in Linux for handling G.007 device 
    regulation. Only one port can be read from, written to at a time. That is, 
    ser.close() must be called before talking to a different piece of hardware 
    with the ser.open() command.
    """

    def set_baud(self, baudrate):
        """
        Sets serial port's baud rate as defined in configuration file.
        """
        self.ser.setBaudrate(baudrate)
        #log.debug("---\t-\t--> Set serial port baud rate to %i for %s" % \
        # (baudrate, self.device_bauds[baudrate]))

    def flush_input(self):
        """
        Flush the input buffer of the serial port.
        """
        log.debug("---\t-\t--> SR: flush serial port input buffer")
        self.ser.flushInput()

    def write_serial(self, data):
        "Flush input buffer, then write string data to serial port."
        self.ser.flushInput()
               #log.debug("---\t-\t--> Flush Input")
        self.ser.write(data)
               #log.debug("---\t-\t--> Write data")

    def parse_read_string(self, write_string, find_string, find_string_size):
        """
        Will read and parse string responses which return program code from \
        the device.
        """

        self.ser.flushInput()
        read_string_char = '-1'

        while read_string_char == '-1':
            self.write_serial(write_string)
                  read_chars = self.read_serial(find_string_size)
            read_string = read_chars.find(find_string)
                  read_string_char = str(read_string)
            time.sleep(0.001)
            #log.debug("---\t-\t--> Read Chars %s" % (read_chars))
        return read_chars

    def read_serial(self, num_expected):
        """
        Return the number of chars in the receive buffer and compare it 
        to expected character number passed as an argument.
        """

        total_received = 0
        read_chars = ""    
    
        while total_received < num_expected:
            iw = self.ser.inWaiting()

            if iw > num_expected:
                iw = num_expected
            read_chars = read_chars + self.ser.read(iw)
            total_received += iw
            time.sleep(0.01)
            #log.debug("---\t-\t--> READCHAR %s" % (read_chars))
            #log.debug("---\t-\t--> Num expected %s" % (num_expected))
        return read_chars

    def read_serial_r(self, num_expected):
        """
        Return the number of chars in the receive buffer and compare it to 
        expected character number passed as an argument.
        """

        read_chars = ""    
        iw = self.ser.inWaiting()
        read_chars = read_chars + self.ser.read(iw)
        # log.debug("---\t-\t--> READCHAR %s" % (read_chars))
        # log.debug("---\t-\t--> Num expected %s" % (iw))
        return read_chars

    def __del__(self):
        """
        Destructs serial port object - it closes any open session.
        """
        self.ser.close()

