"""
-------------------------------------------------------------------------------- 
 Author: Richard Terry.
 Date: March 5, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.
 
 Purpose: This program contains the complete code for class Tel_net, containing
 telnet communication subroutines in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file. 
------------------------------------------------------------------------------- 
"""

import sys
import telnetlib

class Tel_net:

	global telnet_session

	def __init__(self, logger=None):
		"Initialize telnet connection object with default parameters"

		if logger is not None:			# if defined, place logger into Tel_net
			self.logging = logger

		maestro_address = '10.0.0.56'
		self.telnet_session = telnetlib.Telnet(maestro_address)
		m = self.telnet_session.read_until('>')	# search return string for maestro prompt
		self.telnet_session.write('\r')
		d = self.telnet_session.read_until('>')	# search return string for > 

		if logger:
			self.logging.info("---\t-\t--> Initialized telnet connection to address %s" % maestro_address)

	def parse_read_string(self, write_string, find_string):
		"Will read and parse string responses which return program code from the device"
		
		self.telnet_session.write(write_string + '\r')
		read_string = self.telnet_session.read_until(find_string)	# search return string for > 
                return read_string
                
	def __del__(self):
		"Destructs telnet conncetion object - it closes any open session"
		self.telnet_session.close()

