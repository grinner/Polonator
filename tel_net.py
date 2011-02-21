"""
-------------------------------------------------------------------------------- 
 Author: Richard Terry.
 Date: March 5, 2008.
 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.
 
 Purpose: This program contains the complete code for class Tel_net, containing
 telnet communication subroutines in Python.
------------------------------------------------------------------------------- 
"""

import sys
import telnetlib


class Tel_net:
    global telnet_session

    def __init__(self):
        #maestro_address = '10.0.225.247'
        maestro_address = 'controller'

        self.telnet_session = telnetlib.Telnet(maestro_address)
        m = self.telnet_session.read_until('>') #search return string for maestro prompt
        self.telnet_session.write('\r')
        d = self.telnet_session.read_until('>') #search return string for > 
    def parse_read_string(self, write_string, find_string):
        
        self.telnet_session.write(write_string + '\r')
        read_string = self.telnet_session.read_until(find_string) #search return string for > 
        return read_string

    def __del__(self):
        self.telnet_session.close()

