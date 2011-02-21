"""-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: February 13, 2008.
 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.
 
 Purpose: This program contains the complete code for module Biochem, 
 containing re-occurring biochecmistry subroutines in Python.

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file. 
------------------------------------------------------------------------------- 
""" 

import xlrd

book = xlrd.open_workbook("G007 Fluidic Volumes.xls")	# read in Excel file into 'xlrd' object
sh = book.sheet_by_index(0)														# create handle for first Excel sheet 

#--------------------------------------------------------------------------------------#
#								                  INTERNAL VOLUMES				  													 #
#--------------------------------------------------------------------------------------#

i_volumes = {}  # create dictionary for internal tubing volumes

for row in range(2, 44):

	from_row = sh.cell_value(rowx=row, colx=0)
	to_row = sh.cell_value(rowx=row, colx=1)

	# From reagent block into manifold

	block_to_manifold = sh.cell_value(rowx=row, colx=2)	
	tube_area_1 = sh.cell_value(rowx=row, colx=3)

	tube_volume_1 = block_to_manifold * tube_area_1     # tube volume 1

	# Manifold layer transition

	manifold_transition = sh.cell_value(rowx=row, colx=5)
	hole_area = sh.cell_value(rowx=row, colx=6)

	transition_volume = manifold_transition * hole_area  # transition_volume 

	# Manifold path length

	manifold_path_length = sh.cell_value(rowx=row, colx=8)
	path_area = sh.cell_value(rowx=row, colx=9)

	path_volume = manifold_path_length * path_area       # path volume 

	# From manifold to valve

	manifold_to_valve = sh.cell_value(rowx=row, colx=11)
	tube_area_2 = sh.cell_value(rowx=row, colx=12)

	tube_volume_2 = manifold_to_valve * tube_area_2			 # tube volume 2

	#--------------------------- Volume dictionary creation ------------------------------

	total_volume = tube_volume_1 + transition_volume + path_volume + tube_volume_2	# total volume sum
	
	if not i_volumes.has_key(to_row):
		i_volumes[to_row] = {}

	"""
	try:
		i_volumes[to_row]
	except KeyError:
		i_volumes[to_row] = {}
	"""

	i_volumes[to_row][from_row] = total_volume

print "\n--> TESTING INTERNAL:", i_volumes

#--------------------------------------------------------------------------------------#
#								                   EXTERNAL VOLUMES						 												 #
#--------------------------------------------------------------------------------------#

e_volumes = {}  # create dictionary for external tubing volumes

for row in range(47, 62):

	from_row = sh.cell_value(rowx=row, colx=0)
	to_row = sh.cell_value(rowx=row, colx=1)

	# Tubing run length

	tubing_run = sh.cell_value(rowx=row, colx=2)	
	cross_sectional_area = sh.cell_value(rowx=row, colx=4)

	total_volume = tubing_run * cross_sectional_area  # tubing path volume

	#--------------------------- Volume dictionary creation ------------------------------
 
	if not e_volumes.has_key(to_row):
		e_volumes[to_row] = {}

	e_volumes[to_row][from_row] = total_volume

print "\n\n--> TESTING EXTERNAL:", e_volumes

#--------------------------------------------------------------------------------------#
#							                   MIXING CHAMBER VOLUMES													 			 #
#--------------------------------------------------------------------------------------#

for row in range(63, 65):

	from_row = sh.cell_value(rowx=row, colx=0)
	to_row = sh.cell_value(rowx=row, colx=1)
 
	if not e_volumes.has_key(to_row):
		e_volumes[to_row] = {}

	e_volumes[to_row][from_row] = sh.cell_value(rowx=row, colx=5)

print "\n\n--> TESTING MIXER:", e_volumes
print "\n--> FINISHED VOLUME TESTING\n"

