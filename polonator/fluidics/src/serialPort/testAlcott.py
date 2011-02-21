
"""\
Scan for serial ports. Linux specific variant that also includes USB/Serial
adapters.

Part of pySerial (http://pyserial.sf.net)#! /usr/bin/env python
(C) 2009 <cliechti@gmx.net>
"""

import serial
import glob

"""
     The Port mapping is as follows
     
"""
global ser

def getSerialPortList():
    """
          returns a list of serial ports 
          scan for available ports. return a list of device names.
    """
    return glob.glob('/dev/ttyS*') + glob.glob('/dev/ttyUSB*')
#end def

def openAlcottSerial():
    """
     
    """
    global ser
    #ser = serial.Serial('/dev/ttyUSB0', 38400, timeout=1)
    ser = serial.Serial('/dev/ttyUSB0', 9600, parity=serial.PARITY_NONE, bytesize=8, stopbits=1) 
    #ser = serial.serial_for_url('/dev/ttyUSB0', 9600, parity=serial.PARITY_NONE, bytesize=8, stopbits=1) 
    #ser.write('@')
    #x = int(ser.read(1))          # read one byte
    print ser.isOpen()
    #print x
    #s = ser.read(10)        # read up to ten bytes (timeout)
    #
    #line = ser.readline()   # read a '\n' terminated line
# end def

def closeAlcottSerial():
    global ser
    ser.close()
# end def 

def buzzerSound(string_buzz):
    global ser
    if string_buzz == 'ON':
        print "buzzer on\n"
        ser.write("@BZR ON\n")  
    elif string_buzz == 'OFF':
        print "buzzer off\n"
        ser.write("@BZR OFF\n") 
    else:
        print "buzzer off\n"
        ser.write("@bzr OFF\n")
    #for c in str_write:
     #   ser.write(c)
    #end for 
# end def

if __name__=='__main__':
    print "Found ports:"
    for name in getSerialPortList():
        print name
        
        
