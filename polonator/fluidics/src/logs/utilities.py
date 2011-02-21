#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: March 18, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: contains utility functions for log-data post-processing. 

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import os
import re
import sys
import glob

#---------------------------------------------------------------------------------------
#				               GREP FUNCTION DEFINITION
#---------------------------------------------------------------------------------------

def grep(pattern, f):
	"""Given a list of files or standard input to read, grep searches for lines of text 
	that match one or many regular expressions, and outputs only the matching lines."""

	search = re.compile(pattern).search
	for line in open(f):
		if search(line):
			print line[:-1]

#---------------------------------------------------------------------------------------
#				               PARSER FUNCTION DEFINITION
#---------------------------------------------------------------------------------------

def parser(pattern, f=None):
	"""Retrieves the last log-file modified or opens a specified file, then parses it for
    given pattern, and finally generates an archive of pattren containing line-logs."""
	
	print '\n--> Pattern-log file generation started'	# pattern log parsing start.

	if f:
		mod_last = f
	else:
		files = glob.glob("biochemistry_*.log")

		mod_date = 0
		for f in files:									# find last modified file
			if mod_date < os.stat(f).st_mtime:
				mod_last = f	
				mod_date = os.stat(f).st_mtime	

	line_list = []
	search = re.compile(pattern).search					# set pattern search parameter

	for line in open(mod_last):							# loop through all lines in the file
		if search(line):
			line_list.append(line[:-1].strip() + '\n')	# store lines with patterns in container	

	if not len(line_list) == 0:
		pattern_log = open(pattern + '_' + mod_last, 'a')    # open up log file to be written

		for i in range(len(line_list)):
			pattern_log.write(line_list.pop(0))			# write line into file where pattern found from container

		pattern_log.close()								# close log-file

		print '--> Finished: parsed file [%s] for pattern [%s]\n' % (str(mod_last), pattern)
	else:
		print '--> Finished: no match found in file [%s] for pattern [%s]\n' % (mod_last, pattern)

