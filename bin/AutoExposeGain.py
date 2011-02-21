## ========================================================================================
##
## Polonator G.007 Image Acquisition Software
##
## Church Lab, Harvard Medical School
## Written by Richard C Terry
##
## AutoExposeGain.py: automated routine to determine EM gain for each fluorescent channel
##
## Release 1.0 -- 07-08-2009
##
## This software may be modified and re-distributed, but this header must appear
## at the top of the file.
##
## ========================================================================================
##

import sys
import CameraFunctions
import MaestroFunctions
import LoggerFunctions
import tel_net
import random
import time
import subprocess


class Autoexpose:
	global camera
	global telnet
	global maestro
	global device_filters
	global autoex_gains

	def __init__(self):
		tt=1
		#self.camera = CameraFunctions.Camera_Functions()
		#self.maestro = MaestroFunctions.Maestro_Functions()
		#self.telnet = tel_net.Tel_net()
		#self.logger = LoggerFunctions.Logger_Functions('../logs/autoexpose-log')

	def autoe(self):
		p = subprocess.Popen('/home/polonator/G.007/G.007_acquisition/PolonatorUtils gotostagealignpos 0 0',  shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		stdout_value, stderr_value = p.communicate()
		print 'stdout_value \n' , stdout_value
		autoex_gains=[0,0,0,0]
		fluors = ['cy5','fam','cy3','txred']
		for_index=0
		for x in fluors:
			image_mean = str(0)
			expose = 0
			while_index = 0
			print x
			while int(image_mean) < 4000 and expose < 220:
				expose = 90+(while_index*10)
				print '../PolonatorUtils snap ' + x + ' 0.035 ' + str(expose)
				p = subprocess.Popen('/home/polonator/G.007/G.007_acquisition/PolonatorUtils snap ' + x + ' 0.035 ' + str(expose),  shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				stdout_value, stderr_value = p.communicate() 
				print 'stdout_value \n' , stdout_value
				image_mean_stdout_value = stdout_value.find('Image mean: ') + 12
				print 'image mean stdout_value' , image_mean_stdout_value, '\n'
				image_mean = stdout_value[image_mean_stdout_value:image_mean_stdout_value+4]
				print 'image mean' , image_mean
				print 'expose', expose
				while_index = while_index + 1
				time.sleep(1)
			autoex_gains[for_index]=expose
			for_index = for_index + 1
			time.sleep(1)
		print 'fluors ', fluors, ' gains ', autoex_gains
		print "Done"
		return autoex_gains
