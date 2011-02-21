#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: May 8, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: given function command, this utility program performs any device 
 method consisting of:

		 1. temperature_control (temperature controller)
		 2. syringe_pump (syringe pump)
		 3. rotary_valve (rotary valve)

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import sys
import time

#----------------------------- Input argument handling ---------------------------------

if len(sys.argv) == 1:
	print """\n
     ----------------------------------------------------------------------   
     -                                                                    -      
     -               WELCOME TO THE DEVICE UTILITY PROGRAM                -
     -                                                                    -
     -                Usage: python device_utils.py method                -
     -                                                                    -
     ----------------------------------------------------------------------

     |  Methods defined here:
     |
     |  --------- Mux class --------- 
     |  
     |  discrete_valve4_close(self)
     |      Sets valve V4 to OFF state
     |  
     |  discrete_valve4_open(self)
     |      Sets valve V4 to ON state
     |  
     |  discrete_valve5_close(self)
     |      Sets valve V5 to OFF state
     |  
     |  discrete_valve5_open(self)
     |      Sets valve V5 to ON state
     |  
     |  discrete_valve6_close(self)
     |      Sets valve V6 to OFF state
     |  
     |  discrete_valve6_open(self)
     |      Sets valve V6 to ON state
     |  
     |  discrete_valve7_close(self)
     |      Sets valve V7 to OFF state
     |  
     |  discrete_valve7_open(self)
     |      Sets valve V7 to ON state
     |  
     |  mixer_OFF(self)
     |      Mixing in mixer: OFF
     |  
     |  mixer_ON(self)
     |      Mixing in mixer: ON
     |  
     |  set_to_reagent_block_cooler(self)
     |      Communication channel set to reagent block cooler
     |  
     |  set_to_rotary_valve1(self)
     |      Communication channel set to rotary valve 1
     |  
     |  set_to_rotary_valve2(self)
     |      Communication channel set to rotary valve 2
     |  
     |  set_to_rotary_valve3(self)
     |      Communication channel set to rotary valve 3
     |  
     |  set_to_syringe_pump(self)
     |      Communication channel set to syringe pump
     |  
     |  set_to_temperature_control1(self)
     |      Communication channel set to temperature controller 1
     |  
     |  set_to_temperature_control2(self)
     |      Communication channel set to temperature controller 2
     |
     |  --------- Temperature control class --------- 
     |  
     |  get_temperature(self)
     |      Gets temperature sensor 1 reading, a float - register [100]
     |  
     |  set_control_off(self)
     |      Clears RUN flag in regulator, so main output is blocked
     |  
     |  set_control_on(self)
     |      Sets RUN flag in regulator, so main output is opened
     |  
     |  set_temperature(self, temperature)
     |      Sets main temperature reference (C), a float - register [0]
     |
     |  --------- Syringe pump class --------- 
     |
     |  initialize_syringe(self)
     |      Initializes syringe pump with default operation settings
     |  
     |  set_absolute_volume(self, absolute_volume)
     |      Sets syringe pump absolute volume (an integer) in ragne of 0-1000, where 0 is
     |      the syringe initial position and the maximum filling volume is the stroke of 
     |      the syringe (1000 ul)
     |  
     |  set_speed(self, speed)
     |      Sets syringe pump move speed (an integer) in range of 0-40, where the 
     |      maximum speed is 0 equivalent to 1.25 strokes/second = 1250 ul/s
     |  
     |  set_syringe_valve_position(self, valve_position)
     |      Sets to given syringe pump valve position, an integer
     |
     |  --------- Rotary valve class --------- 
     |
     |  set_rotary_valve_position(self, valve_position)
     |      Switch valve to given port on rotary valve, an integer\n\n"""

	sys.exit()

elif len(sys.argv) < 2:
	print '\n--> Error: not correct input!\n--> Usage: python device_utils.py method\n'
	sys.exit()

else:

	import ConfigParser						# Import configuration parser class.
	from logger import Logger				# Import logger class.
	from biochem import Biochem				# Import biochecmistry class.

	#--------------------- G.007 fluidics sub-system initialization ------------------------

	config = ConfigParser.ConfigParser()
	config.readfp(open('config.txt'))

	t0 = time.time()                # get current time
	logger = Logger(config)         # initialize logger object

	print '\n'

	biochem = Biochem('WL1', 0, logger)  # Initialize biochemistry object - cycle-name and flowcell-number need to be passed.

	logger.info('---\t-\t--> Started %s method execution - device_utils.py' % sys.argv[1])
	method = sys.argv[1]  # set method to second argument

	#---------------------------------------------------------------------------------------
	#								TEMPERATURE CONTROL					
	#---------------------------------------------------------------------------------------

	if method == 'set_to_temperature_control1':
		biochem.mux.set_to_temperature_control1()

	elif method == 'set_to_temperature_control2':
		biochem.mux.set_to_temperature_control2()

	elif method == 'get_temperature':
		c_temp = biochem.temperature_control.get_temperature()
		print "INFO\t---\t-\t--> Current flowcell temperature: %0.2f C" % c_temp

	elif method == 'set_control_on':
		biochem.temperature_control.set_control_on()

	elif method == 'set_control_off':
		biochem.temperature_control.set_control_off()

	elif method == 'set_temperature':
		print "INFO\t---\t-\t--> Please, enter set point temperature [integer]: ",
		set_temp = int(sys.stdin.readline().strip())  # use stdin explicitly and remove trailing newline character
		biochem.temperature_control.set_temperature(set_temp)

	elif method == 'set_to_reagent_block_cooler':
		biochem.mux.set_to_reagent_block_cooler()

	#---------------------------------------------------------------------------------------
	#			         			ROTARY VALVE CONTROL
	#---------------------------------------------------------------------------------------

	elif method == 'set_to_rotary_valve1':
		biochem.mux.set_to_rotary_valve1()

	elif method == 'set_to_rotary_valve2':
		biochem.mux.set_to_rotary_valve2()

	elif method == 'set_to_rotary_valve3':
		biochem.mux.set_to_rotary_valve3()

	elif method == 'set_rotary_valve_position':
		print "INFO\t---\t-\t--> Please, enter valve position (1-9) to be set [integer]: ",
		position = int(sys.stdin.readline().strip())  # use stdin explicitly and remove trailing newline character
		biochem.rotary_valve.set_valve_position(position)

	#---------------------------------------------------------------------------------------
	#								SYRINGE PUMP CONTROL
	#---------------------------------------------------------------------------------------

	elif method == 'set_to_syringe_pump':
		biochem.mux.set_to_syringe_pump()

	elif method == 'initialize_syringe':
		biochem.syringe_pump.initialize_syringe()

	elif method == 'absolute_volume':
		print "INFO\t---\t-\t--> Please, enter absolute syringe volume (0-1000) to be set [integer]: ",
		volume = int(sys.stdin.readline().strip())  # use stdin explicitly and remove trailing newline character
		biochem.syringe_pump.set_absolute_volume(volume)

	elif method == 'set_speed':
		print "INFO\t---\t-\t--> Please, enter syringe speed (0-40) to be set [integer]: ",
		speed = int(sys.stdin.readline().strip())  # use stdin explicitly and remove trailing newline character
		biochem.syringe_pump.set_speed(speed)

	elif method == 'set_syringe_valve_position':
		print "INFO\t---\t-\t--> Please, enter valve position (1-9) to be set [integer]: ",
		position = int(sys.stdin.readline().strip())  # use stdin explicitly and remove trailing newline character
		biochem.syringe_pump.set_valve_position(position)

	#---------------------------------------------------------------------------------------
	#			        		   DISCRETE VALVE CONTROL
	#---------------------------------------------------------------------------------------

	elif method == 'discrete_valve4_open':
		biochem.mux.discrete_valve4_open()

	elif method == 'discrete_valve4_close':
		biochem.mux.discrete_valve4_close()

	elif method == 'discrete_valve5_open':
		biochem.mux.discrete_valve5_open()

	elif method == 'discrete_valve5_close':
		biochem.mux.discrete_valve5_close()

	elif method == 'discrete_valve6_open':
		biochem.mux.discrete_valve6_open()

	elif method == 'discrete_valve6_close':
		biochem.mux.discrete_valve6_close()

	elif method == 'discrete_valve7_open':
		biochem.mux.discrete_valve7_open()

	elif method == 'discrete_valve7_close':
		biochem.mux.discrete_valve7_close()

	#---------------------------------------------------------------------------------------
	#			         		  MIXING CHAMBER CONTROL
	#---------------------------------------------------------------------------------------

	elif method == 'mixer_ON':
		biochem.mux.mixer_ON()
		time.sleep(5)

	elif method == 'mixer_OFF':
		biochem.mux.mixer_OFF()

	else:
	 print '\nINFO\t---\t-\t--> Error: not correct method input!\n--> Double check method name (1st argument)\n'
	 sys.exit()

	#-------------------------- Duration of biochemistry test ------------------------------

	delta = (time.time() - t0) / 60  # Calculate elapsed time for flowcell flush.
	logger.info('---\t-\t--> Finished %s method execution - duration: %0.2f minutes\n\n' % (method, delta))

