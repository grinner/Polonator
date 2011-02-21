#!/usr/local/bin/python

"""
-------------------------------------------------------------------------------- 
 Author: Mirko Palla.
 Date: March 18, 2008.

 For: G.007 polony sequencer design [fluidics software] at the Church Lab - 
 Genetics Department, Harvard Medical School.

 Purpose: retrieves the last log-file modified and parses it for given pattern,
 and finally generates an archive of pattern containing line-logs. 

 This software may be used, modified, and distributed freely, but this
 header may not be modified and must appear at the top of this file.
------------------------------------------------------------------------------- 
"""

import os
import re
import sys
import glob

#-------------------------------- pattern parsing ---------------------------------
	
print '\n--> Pattern-log file generation started - pattern_log.py'	# pattern log parsing start.

if len(sys.argv) < 2:
	print '--> Error: not correct input!\n--> Usage: python parse_log.py pattern [parse_file]\n'
	sys.exit()

if len(sys.argv) == 3:
	mod_last = sys.argv[2]

else:
	files = glob.glob("biochemistry_*.log")

	mod_date = 0
	for f in files:										# find last modified file
		if mod_date < os.stat(f).st_mtime:
			mod_last = f	
			mod_date = os.stat(f).st_mtime	

line_list = []
search = re.compile(sys.argv[1]).search				# set pattern search parameter

for line in open(mod_last):							# loop through all lines in the file
	if search(line):
		line_list.append(line[:-1].strip() + '\n')	# store lines with patterns in container	

if not len(line_list) == 0:
	pattern_log = open(sys.argv[1] + '_' + mod_last, 'a')    # open up log file to be written

	for i in range(len(line_list)):
		pattern_log.write(line_list.pop(0))			# write line into file where pattern found from container

	pattern_log.close()								# close log-file

	print '--> Finished: parsed file [%s] for pattern [%s]\n' % (str(mod_last), sys.argv[1])
else:
	print '--> Finished: no match found in file [%s] for pattern [%s]\n' % (mod_last, sys.argv[1])

