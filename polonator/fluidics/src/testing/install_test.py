#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: March 19, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: this program checks proper installation of all fluidics sub-system 
 devices, i.e., it switches all discrete valves off and on (and vica and versa),
 switches all rotary valves from port position 1 to 10 in a step-wise manner, 
 switches ports on the 9-port syringe pump similarly to protocol described for 
 rotary valves and also performs a full-stroke syringe movement twice. Further-
 more it turns the mixer on and performs mixing for 10 seconds, then it turns 
 the mixer off. And finally it turns "temperature controller 1" on, ramps up the 
 flowcell temperature to 15 C, then turns it off. Similarly, this procedure is 
 repeated for "temperature controller 2" and "reagent block cooler".  

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import sys
import time
import ConfigParser						# Import configuration file parser class.

from logger import Logger					# Import logger class.
from serial_port import Serial_port				# Import serial port class.
from mux import Mux						# Import serial port class.

from syringe_pump import Syringe_pump				# Import syringe_pump class.
from rotary_valve import Rotary_valve				# Import rotary valve class.
from temperature_control import Temperature_control		# Import temperature controller class.

#import pycallgraph

#--------------------------- Configuration input handling ------------------------------

#pycallgraph.start_trace()	# start process call graph

if len(sys.argv) < 2:
	print '\n--> Error: not correct input!\n--> Usage: python install_test.py config.txt\n'
	sys.exit()

config = ConfigParser.ConfigParser()				# Create configuration file parser object.
config.read(sys.argv[1])					# Fill it in with configuration parameters from file.
logger = Logger(config)						# Initialize logger object.	

#---------------------------- Device(s) initialization ---------------------------------
	
t0 = time.time()							# Get current time.
print '\n'
logger.info('***\t*\t--> Installation testing started - install_test.py')  # Installation test start.

serial_port = Serial_port(config, logger)				# Initialize serial port object.
mux = Mux(logger)							# Initialize mux object.

syringe_pump = Syringe_pump(config, serial_port, logger)		# Initialize syringe pump object.
rotary_valve = Rotary_valve(config, serial_port, logger)		# Initialize rotary valve object.
temperature_control = Temperature_control(config, serial_port, logger)	# Initialize temperature controller object.

#---------------------------------------------------------------------------------------
#			        		   DISCRETE VALVE CONTROL
#---------------------------------------------------------------------------------------

for i in range(0, 2):							# Switche all discrete valves off and on twice.

	mux.discrete_valve4_open()					# Open discrete valve 4.
	time.sleep(1)
	mux.discrete_valve4_close()					# Close discrete valve 4.
	time.sleep(1)

	mux.discrete_valve5_open()					# Open discrete valve 5.
	time.sleep(1)
	mux.discrete_valve5_close()					# Close discrete valve 5.
	time.sleep(1)

	mux.discrete_valve6_open()					# Open discrete valve 6.
	time.sleep(1)
	mux.discrete_valve6_close()					# Close discrete valve 6.
	time.sleep(1)

	mux.discrete_valve7_open()					# Open discrete valve 7.
	time.sleep(1)
	mux.discrete_valve7_close()					# Close discrete valve 7.
	time.sleep(1)

#---------------------------------------------------------------------------------------
#			       ROTARY VALVE CONTROL
#---------------------------------------------------------------------------------------

mux.set_to_rotary_valve1()						# Switch communication to rotary valve 1.

for i in range(1, 11):							# Switch all rotary valves from port position 1 to 10 in a step-wise manner.
	rotary_valve.set_valve_position(i)				# Set ten port valve to given position.
	time.sleep(1)

mux.set_to_rotary_valve2()						# Switch communication to rotary valve 2.

for i in range(1, 11):							# Switch all rotary valves from port position 1 to 10 in a step-wise manner.
	rotary_valve.set_valve_position(i)				# Set ten port valve to given position.
	time.sleep(1)

mux.set_to_rotary_valve3()						# Switch communication to rotary valve 3.

for i in range(1, 11):							# Switch all rotary valves from port position 1 to 10 in a step-wise manner.
	rotary_valve.set_valve_position(i)				# Set ten port valve to given position.
	time.sleep(1)

#---------------------------------------------------------------------------------------
#				SYRINGE PUMP CONTROL
#---------------------------------------------------------------------------------------

# Switch ports on the 9-port syringe pump similarly to protocol described above. 

mux.set_to_syringe_pump()				 		# Switch communication to syringe pump.
syringe_pump.initialize_syringe()					# Initialize syringe pump.

for i in range(1, 10):							# Switch all rotary valves from port position 1 to 10 in a step-wise manner.
	syringe_pump.set_valve_position(i)		 		# Set valve to port 'i'.
	time.sleep(1)

# Performs a full-stroke syringe movement twice.

for i in range(0, 2):							# Switch all rotary valves from port position 1 to 10 in a step-wise manner.
	syringe_pump.set_speed(13)					# Set syringe speed to pull speed (moderately fast).
	syringe_pump.set_absolute_volume(1000) 				# Draw 1000 ul of fluid into syringe.
	syringe_pump.set_speed(0)					# Set syringe speed to eject speed (fastest possible on 0-40 scale).
	syringe_pump.set_absolute_volume(0)				# Empty syringe contents.
	time.sleep(1)

#---------------------------------------------------------------------------------------
#			       MIXING CHAMBER CONTROL
#---------------------------------------------------------------------------------------

mux.mixer_ON()								# Turn mixer on.
time.sleep(10)								# Performs mixing for 10 seconds.
mux.mixer_OFF()								# Turn mixer off.

#---------------------------------------------------------------------------------------
#				TEMPERATURE CONTROL					
#---------------------------------------------------------------------------------------

#------------------------- Steady-state temperature waiting ----------------------------

def wait_for_SS(target):
	"""Waits until steady-state temperature is reached, or exits wait block if ramping
	time exceeds timeout parameter set in manually here."""

	time_limit = 10					# set time limit for temperature setting

	t0 = time.time()  				# get current time
	tc = temperature_control.get_temperature() 	# get current flowcell temperature

	while abs(target - tc) > 1:

		tc = temperature_control.get_temperature()
		time.sleep(1)

		delta = time.time() - t0 # elapsed time in seconds
		sys.stdout.write('TIME---\t-\t---> Elapsed time: ' + str(delta) + ' second \n')
		sys.stdout.flush()
                         
		if delta > time_limit * 60:
			print "--> Time limit %s exceeded -> [current: %f, target: %f] C" % (time_limit, tc, target)
			break

	elapsed = (time.time() - t0) / 60
	print "--> Time to set temperature: %0.2f minutes" % elapsed

#---------------------------------------------------------------------------------------

target = 15						# Set target temperature to 15 C.

mux.set_to_temperature_control1()			# Switch communication to temperature controller 1.
temperature_control.set_control_on()			# Set RUN flag command.
temperature_control.set_temperature(target)	    	# Set register 0 to 'temperature', a float.
wait_for_SS(target)					# Wait until target temperature is reached.
temperature_control.set_control_off()			# Clear RUN flag command.


mux.set_to_temperature_control2()			# Switch communication to temperature controller 2.
temperature_control.set_control_on()			# Set RUN flag command.
temperature_control.set_temperature(target)	    	# Set register 0 to 'temperature', a float.
wait_for_SS(target)					# Wait until target temperature is reached.
temperature_control.set_control_off()			# Clear RUN flag command.

#---------------------------------------------------------------------------------------
#			     REAGENT BLOCK CONTROL					
#---------------------------------------------------------------------------------------

mux.set_to_reagent_block_cooler()			# Switch communication to reagent block cooler.
temperature_control.set_control_on()			# Set RUN flag command.
temperature_control.set_temperature(target)	    	# Set register 0 to 'temperature', a float.
wait_for_SS(target)					# Wait until target temperature is reached.
temperature_control.set_control_off()			# Clear RUN flag command.

#-------------------------- Duration of device test ------------------------------

delta = (time.time() - t0) / 60				# Calculate elapsed time for polony sequencing cycles.
logger.warn("***\t*\t--> Finished installation test protocol - duration: %0.2f minutes\n" % delta)

#pycallgraph.make_dot_graph('test.png')	# Finish graphing and generate file.
