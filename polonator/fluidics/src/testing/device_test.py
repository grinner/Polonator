#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: February 12, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: performs a function testing of G.007 fluidics device communication
 consisting of:

		 1. temperature_control (temperature controller)
		 2. syringe_pump (syringe pump)
		 3. rotary_valve (rotary valve) 

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import sys
import time
import serial
import ConfigParser						# Import configuration file parser class.

from logger import Logger					# Import logger class.
from serial_port import Serial_port				# Import serial port class.
from mux import Mux						# Import serial port class.
from biochem import Biochem					# Import biochemistry class.

from syringe_pump import Syringe_pump				# Import syringe_pump class.
from rotary_valve import Rotary_valve				# Import rotary valve class.
from temperature_control import Temperature_control		# Import temperature controller class.

#--------------------------- Configuration input handling ------------------------------

if len(sys.argv) < 2:
	print '\n--> Error: not correct input!\n--> Usage: python polony_sequencing.py config.txt\n'
	sys.exit()

config = ConfigParser.ConfigParser()				# Create configuration file parser object.
config.read(sys.argv[1])					# Fill it in with configuration parameters from file.
logger = Logger(config)						# Initialize logger object.	

#---------------------------- Device(s) initialization ---------------------------------
	
t0 = time.time()						# Get current time.
print '\n'
logger.info('***\t*\t--> Device testing started - test.py')	# Test start.

serial_port = Serial_port(config, logger)			# Initialize serial port object.
mux = Mux(logger)						# Initialize mux object.

syringe_pump = Syringe_pump(config, serial_port, logger)		# Initialize syringe pump object.
rotary_valve = Rotary_valve(config, serial_port, logger)		# Initialize rotary valve object.
temperature_control = Temperature_control(config, serial_port, logger)	# Initialize temperature controller object.

#---------------------------------------------------------------------------------------
#								TEMPERATURE CONTROL					
#---------------------------------------------------------------------------------------

mux.set_to_temperature_control1()				# Switch communication to temperature controller 1.
mux.set_to_temperature_control2()				# Switch communication to temperature controller 2.

temperature_control.set_control_on()				# Set RUN flag command.
temperature_control.set_control_off()				# Clear RUN flag command.

temperature_control.set_temperature(30)			# Set register 0 to 'temperature', a float.
t = temperature_control.get_temperature()			# Get current temperature reading of heat-spreader.

print "Current temperature: %0.2f C\n" %t

#---------------------------------------------------------------------------------------
#								REAGENT BLOCK CONTROL					
#---------------------------------------------------------------------------------------

mux.set_to_reagent_block_cooler()				# Switch communication to reagent block cooler.
biochem.reagent_block_init()					# Set reagent block cooler to 4C.

#---------------------------------------------------------------------------------------
#								SYRINGE PUMP CONTROL
#---------------------------------------------------------------------------------------

mux.set_to_syringe_pump()			 		# Switch communication to syringe pump.
syringe_pump.initialize_syringe()				# Initialize syringe pump.

syringe_pump.set_valve_position(0)			 	# Set valve to port 'i'.
syringe_pump.set_speed(20)					# Valid speed range 0 to 40, the maximum speed is 0.
syringe_pump.set_absolute_volume(1000)		  		# Draw [1] ul of fluid into syringe.

#---------------------------------------------------------------------------------------
#			         			ROTARY VALVE CONTROL
#---------------------------------------------------------------------------------------

mux.set_to_rotary_valve1()					# Switch communication to rotary valve 1.
mux.set_to_rotary_valve2()					# Switch communication to rotary valve 2.
mux.set_to_rotary_valve3()					# Switch communication to rotary valve 3.

rotary_valve.set_valve_position(6)				# Set ten port valve to given position.

#---------------------------------------------------------------------------------------
#			        		   DISCRETE VALVE CONTROL
#---------------------------------------------------------------------------------------

mux.discrete_valve4_open()					# Open discrete valve 4.
mux.discrete_valve4_close()					# Close discrete valve 4.

mux.discrete_valve5_open()					# Open discrete valve 5.
mux.discrete_valve5_close()					# Close discrete valve 5.

mux.discrete_valve6_open()					# Open discrete valve 6.
mux.discrete_valve6_close()					# Close discrete valve 6.

mux.discrete_valve7_open()					# Open discrete valve 7.
mux.discrete_valve7_close()					# Close discrete valve 7.

#---------------------------------------------------------------------------------------
#			         		  MIXING CHAMBER CONTROL
#---------------------------------------------------------------------------------------

mux.mixer_ON()							# Turn mixer on.
time.sleep(5)
mux.mixer_OFF()							# Turn mixer off.

#-------------------------- Duration of device test ------------------------------

delta = (time.time() - t0) / 60					# Calculate elapsed time for polony sequencing cycles.
logger.warn("***\t*\t--> Finished device test protocol - duration: %0.2f minutes\n\n" % delta)

