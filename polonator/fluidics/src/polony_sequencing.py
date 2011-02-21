#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: March 19, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: polony_sequencing.py performs a polony sequencing biochemistry 
 consisting of given number of cycles of iteration (26)

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import sys
from biochem import Biochem		# Import biochecmistry class.

#--------------------------- Configuration input handling ------------------------------

if len(sys.argv) < 3:
	print '\n--> Error: not correct input!\n--> Usage: python polony_sequencing.py cycle-name flowcell-number\n'
	sys.exit()

#--------------------- G.007 fluidics sub-system initialization ------------------------

t0 = time.time()					# Get current time.
biochem = Biochem(sys.argv[1], int(sys.argv[2]))	# Initialize biochemistry object - cycle-name and flowcell-number need to be passed.

#-------------------------- Alternating cycle iterations -------------------------------

biochem.run()						# Run polony sequencing cycle(s).
