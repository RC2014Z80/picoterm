#!/usr/bin/env python3
""" test_receive.py - Just print the data (HEX repr) as received over the serial line.
"""

__version__ = '0.1'

import sys
import serial
import time
from random import choice

def show_help():
	print( '                  by Meurisse D.')
	print( 'USAGE:')
	print( '  ./test_receive.py <device> -h' )
	print( '' )
	print( '<device>  : serial device connected to PicoTerm' )
	print( '-h        : display this help.')
	print( '' )

def get_args( argv ):
	""" Process argv and extract: output, source, device, user parameters """
	r = { 'device' : None } # unamed, -r, -hex 0x0000
	used = [] # list of used entries in argv
	used.append(0) # item #0 is the script name
	unamed = [] # unamed parameters

	# Locate the unamed parameter
	for i in range( len(argv) ):
		if i in used:
			continue
		else:
			unamed.append( argv[i] )
	# First unamed is the device
	if len(unamed) > 0:
		r['device'] = unamed[0]

	# Sanity check
	if r['device']==None:
		raise Exception('missing device')

	return r


class SerialHelper:
	def __init__( self, args ):
		self.args = args
		self.ser = serial.Serial(args['device'], 115200, timeout=0.500 ) # 100ms

	def write_str( self, str ):
		""" Generic write that replaces \ESC and \0x1b in unicode string """
		self.ser.write( str.replace('\ESC',chr(27)).encode('ASCII') )

	def write_byte( self, val ):
		assert 0<= val <= 255, "Byte value %s out-of-scope" % val
		self.ser.write( bytes([val]) )

	def readline( self ):
		""" try to read a kube (at least until the serial timeout) """
		return self.ser.readline()


if __name__ == '__main__':
	print( '== PicoTerm Receiver Test %s ==' % __version__ )
	print( 'Show terminal data sent by picoterm when stroking the USB keyboard. ')
	print( 'Content shows in Hexa decimal representation.' )
	print( 'Quite handy to analyse return of special operation (cursor move, etc')
	print( '' )


	if (len( sys.argv )==1) or ('-h' in sys.argv):
		show_help()
		exit(1)

	args = get_args( sys.argv ) # gets 'source', 'raw', 'hex'
	# print( args )
	ser = SerialHelper( args )

	while True:
		d = ser.readline() # returns bytes data
		if (d==None) or (len(d)==0):
			continue
		s1 = [ ('%2s' % hex(c).replace('0x','')).replace(' ', '0').upper() for c in d ]
		print(  '%s  |  %s' % (' '.join(s1),d) )

	print( "That's all folks!" )
