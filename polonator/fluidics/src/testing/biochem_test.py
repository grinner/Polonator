#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: March 19, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: performs function testing of biochemistry object

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import sys
import time
import ConfigParser

from logger import Logger				# Import logger class.
from biochem import Biochem				# Import biochecmistry class.

#--------------------------- Configuration input handling ------------------------------

if len(sys.argv) < 3:
	print '\n--> Error: not correct input!\n--> Usage: python biochem_test.py cycle-name flowcell-number\n'
	sys.exit()

#--------------------- G.007 fluidics sub-system initialization ------------------------

config = ConfigParser.ConfigParser()
config.readfp(open('config.txt'))

t0 = time.time()                # get current time
logger = Logger(config)         # initialize logger object

print '\n'
biochem = Biochem(sys.argv[1], int(sys.argv[2]), logger)  # Initialize biochemistry object - cycle-name and flowcell-number need to be passed.

#---------------------------------------------------------------------------------------
#				 				BIOCHEMISTRY CONTROL
#---------------------------------------------------------------------------------------

logger.info('***\t*\t--> Started biochemistry object testing - biochem_test.py')

#--------------------------------- Initialization --------------------------------------

#biochem.syringe_pump_init()

#biochem.reagent_block_init()				# Set reagent block cooler to constant temperature, 4 Celsius degrees.
#biochem.rotary_valve1_init()				# Initialize reagent start points in ten port rotary valve V1.
#biochem.rotary_valve2_init()				# Initialize reagent start points in ten port rotary valve V2.
#biochem.rotary_valve3_init()				# Initialize reagent start points in ten port rotary valve V3.

#biochem.syringe_pump_init()				# Initializes syringe pump by moving it to zero position and setting speed to 20.
#biochem.mixer_init()					# Prime mixer chamber with Wash 1 as initialization step.
#biochem.ligase_init()					# Initialize ligase/ligase buffer start points in nine port valve V0.

biochem.init()						# Initialize biochemistry sub-system.

#------------------------------- Complex functions -------------------------------------

#biochem.get_config_parameters()				# Retieves all biochemistry and device related configuration parameters configuration file.
#biochem.get_excel_volumes()				# Extracts path length and tube cross-sectional area information from standard Excel file.

#biochem.clean_V1_to_syringe()				# Fills tube path V1 to syringe port 5 with Wash 1 and dumps previous tube content to waste.
#biochem.clean_V2_to_syringe()				# Fills tube path V2 to syringe port 6 with Wash 1 and dumps previous tube content to waste.

#biochem.draw_air_to_syringe()				# Draws a specific volume of air plug to syringe's COM-port.
#biochem.draw_air_to_valve('V3')				# Draws a specific volume of air plug to specified valve COM-port.

#biochem.move_reagent(1000, 20, 4, 30, 7)		# Moves a given volume of reagent from one syringe port to another at different speeds.
#biochem.draw_into_flowcell(1000)			# Draws reagent into flowcell.
#biochem.flush_flowcell(5)				# Flushes flowcell 3-times with Wash 1 or dH20.
#biochem.prime_flowcells()				# Prime both flowcells with Wash 1 as initialization step.

#biochem.set_to_RT()					# Sets temperataure controller to room temperature (28 C).
#biochem.incubate_reagent(1)				# Incubates reagent for given amount of time.
#biochem.wait_for_SS(15)				# Waits until steady-state temperature is reached, or exits wait block if ramping.

#biochem.nonamer_prep('V1', 1)				# Moves nonamer and ligase into mixing chamber.
#biochem.ligase_mix(10)					# Mixes ligase with nonamer in mixing chamber.

"""
biochem.move_reagent(biochem.mixer_to_V5, biochem.pull_speed, 2, biochem.empty_speed, 4) # draw "Wash 1" from mixer through flowcell to waste

biochem.move_reagent(biochem.syringe_1_to_mixer, biochem.pull_speed, 1, biochem.pull_speed, 4)  # clear path syringe port 1 to mixing chamber inlet 1
biochem.move_reagent(biochem.syringe_8_to_mixer, biochem.pull_speed, 8, biochem.pull_speed, 4)  # clear path syringe port 8 to mixing chamber inlet 2
"""

#---------------------------- Biochemistry functions -----------------------------------

#biochem.exo_start()					# Performs biochemistry to prepare polony sequencing run.
#biochem.enzyme_reaction()				# Digests unprotected bead-bound ePCR primers with Exonuclease I using rotary valve 2.

#biochem.strip_chem() 					# Performs chemical stripping protocol for polony sequencing.
#biochem.hyb(4)						# Runs primer hybridization protocol for polony sequencing. 
#biochem.lig_stepup_peg(4, 'V1', 1)			# Runs stepup peg ligation reaction protocol for polony sequencing.
#biochem.cycle_ligation()				# Performs a cycle of polony sequencing biochemistry. 

#-------------------------- Duration of biochemistry test ------------------------------

delta = (time.time() - t0) / 60				# Calculate elapsed time for polony sequencing cycles.
logger.warn('***\t*\t--> Finished biochemistry object testing - duration: %0.2f minutes\n\n' % delta)

