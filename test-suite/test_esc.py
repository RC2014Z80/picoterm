#!/usr/bin/env python3
""" test_esc.py - send various escape sequence over the serial line to
                  test the picoterm capability.
"""

__version__ = '0.1'

import sys
import serial
import time
from random import choice

def show_help():
	print( '                  by Meurisse D.')
	print( 'USAGE:')
	print( '  ./test_esc.py <device> -h' )
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
	# Locate the named parameter
	#for i in range( len(argv) ):
	#	if argv[i] == '-hex':
	#		sAddr = argv[i+1]
	#		try:
	#			if '0x' in sAddr:
	#				r['hex']=eval(sAddr)
	#			else:
	#				r['hex']=int(sAddr)
	#		except:
	#			raise Exception('Invalid interger/hex value for address' % sAddr )
	#		used.append(i)
	#		used.append(i+1)
	#	elif argv[i] == '-r':
	#		r['raw'] = True
	#		used.append(i)

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
		self.ser = serial.Serial(args['device'], 115200, timeout=0 )

	def write_str( self, str ):
		""" Generic write that replaces \ESC and \0x1b in unicode string """
		self.ser.write( str.replace('\ESC',chr(27)).encode('ASCII') )

	def write_byte( self, val ):
		assert 0<= val <= 255, "Byte value %s out-of-scope" % val
		self.ser.write( bytes([val]) )

	def run_test( self, test_name ):
		""" Call a test function and display information locally and remotely """
		print( "Run: %s" % test_name )
		print( get_docstring( test_name ))
		ser.write_str( "Run: %s\r\n" % test_name )
		eval( 'test_%s(ser)' % test_name )

def get_test_names():
	""" List the available test names in the script """
	_names = [ _name.replace('test_','') for _name in globals() if ('test_' in _name) and (_name.index('test_')==0) ]
	return _names

def get_docstring( test_name ):
	""" Extract the documentation of a function """
	_r = ""
	fct_name = 'test_%s' % test_name
	if fct_name in globals():
		_r = globals()[fct_name].__doc__.strip()
	return _r

# --- VARIOUS TESTS ------------------------------------------------------------

def test_nupetscii( ser ):
	""" switch to NupetScii and display full charset """
	ser.write_str( "\ESCF" )
	for row in range( 0x20, 0xFF, 0xF+1 ):
		ser.write_str( "%0x : " % row )
		for col in range( 0x0, 0x0F ):
			ser.write_byte( row+col )
			ser.write_byte( 0x20 ) #space
		ser.write_byte( 13 )
		ser.write_byte( 10 )

def test_ascii( ser ):
	""" switch to ASCII and display full charset """
	ser.write_str( "\ESCG" )
	for row in range( 0x20, 0xFF, 0xF+1 ):
		ser.write_str( "%0x : " % row )
		for col in range( 0x0, 0x0F ):
			ser.write_byte( row+col )
			ser.write_byte( 0x20 ) #space
		ser.write_byte( 13 )
		ser.write_byte( 10 )

def test_lorem( ser ):
	""" Send several paragraphs of Lorem Ipsum """
	sentences = []
	a = "Lorem ipsum dolor sit amet, consectetur adipiscing elit."
	b = "Quisque vitae varius ex, eu volutpat orci."
	c = "Aenean ullamcorper orci et vulputate fermentum."
	d = "Cras erat dui, finibus vel lectus ac, pharetra dictum odio."
	e = "Nullam tempus scelerisque purus, sed mattis elit condimentum nec."
	f = "Etiam risus sapien, auctor eu volutpat sit amet, porta in nunc."
	g = "Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas."
	h = "Proin ipsum purus, laoreet quis dictum a, laoreet sed ligula."
	i = "Integer ultricies malesuada quam."
	j = "Cras vel elit sed mi placerat pharetra eget vel odio."
	k = "Duis ac nulla varius diam ultrices rutrum."

	sentences.append(a)
	sentences.append(b)
	sentences.append(c)
	sentences.append(d)
	sentences.append(e)
	sentences.append(f)
	sentences.append(g)
	sentences.append(h)
	sentences.append(i)
	sentences.append(j)
	sentences.append(k)

	n = 2 # number of paragraphs
	length = [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
	paragraphs = []

	for i in range(1, n + 1):
	    paragraph = ''
	    numofsentences = choice(length)
	    for j in range(1, len(sentences) + 1):
	        sentence = choice(sentences)
	        paragraph = paragraph + sentence
	    paragraphs.append(paragraph)

	for x in paragraphs:
		ser.write_str(x)
		ser.write_str("\r\n") # @ end of paragraph
		ser.write_str("\r\n") # empty line between paragraphs


def test_clearscr( ser ):
	""" Fill screen with content, then clear it from cursor to end of screen"""
	test_lorem( ser )
	ser.write_str( "\ESC[H" ) # move cursor to 0,0
	ser.write_str( "\ESC[0J" ) # clear from cursor to end of screen

def test_cursor_blink( ser ):
	""" Disable cursor blinking to 5 seconds then reactivate it """
	ser.write_str( "Blink OFF\r\n" )
	ser.write_str( "\ESC[?12l" )
	ser.write_str( "Wait 5 sec...\r\n" )
	time.sleep( 5 )
	ser.write_str( "Blink ON\r\n" )
	ser.write_str( "\ESC[?12h" )

def test_cursor_hide( ser ):
	""" Hide the cursor (make it invisible) for 5 seconds then make it visible again """
	ser.write_str( "Hide cursor\r\n" )
	ser.write_str( "\ESC[?25l" )
	ser.write_str( "Wait 5 sec...\r\n" )
	time.sleep( 5 )
	ser.write_str( "Show cursor\r\n" )
	ser.write_str( "\ESC[?25h" )

def test_no_warp( ser ):
	""" Disable WarpAround and send a Lorem Ipsum """
	ser.write_str( "\ESC[?7l" )
	test_lorem( ser )

def test_do_warp( ser ):
	""" complement of no_warp test. Reactivate warp_around and send a Lorem Ipsum """
	ser.write_str( "\ESC[?7h" )
	test_lorem( ser )

def test_clear( ser ):
	""" Write few characters then clear the screen """
	ser.write_str( "Yo man! You should not see this text because screen will be erased :-)")
	ser.write_str("\ESC[2J") # Clear the screen

def test_cursor_save( ser ):
	""" Send Lorem Ipsum. Save cursor position. write 3 lines, restore cursor position. """
	test_clear( ser )
	test_lorem( ser )
	ser.write_str("\ESC[s") # Save the cursor position
	ser.write_str("I will write 3 lines\r\nof text before restoring\r\n" )
	ser.write_str("the cursor position on the first line!" );
	ser.write_str("\ESC[u" ) # Move cursor to previously saved position

def test_move_at_2_3( ser ):
	""" Send Lorem Ipsum, go second line at third character, overwrite with '<The quick brown fox jumps over the lazy dog>' """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3H") # Line 2, Col 3
	ser.write_str("<The quick brown fox jumps over the lazy dog>")

def test_move_at_2_3_v2( ser ):
	""" Send Lorem Ipsum, go second line at third character, overwrite with '<The quick brown fox jumps over the lazy dog>' """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3f") # Line 2, Col 3 (alternate ESC sequence)
	ser.write_str("<The quick brown fox jumps over the lazy dog>")

def test_char_insert( ser ):
	""" Send Lorem Ipsum, go second line at third character, insert 8 spaces """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3H") # Line 2, Col 3
	ser.write_str("\ESC[8@") # Line 2, Col 3

def test_blink( ser ):
	""" Display text with blinking parts and no blinking parts  """
	ser.write_str("This text should NOT BLINK!\r\n")
	ser.write_str("\ESC[5m") # Blink on
	ser.write_str("This text should be blinking!")
	ser.write_str("\ESC[25m") # Blink Off
	ser.write_str(" Except the end of the line.\r\n")
	ser.write_str("This text should NOT BLINK!\r\n")

if __name__ == '__main__':
	print( '== PicoTerm Escape Sequence tester %s ==' % __version__ )
	print( 'Series of test to call on demand to check specific escape ')
	print( 'sequence handling on PicoTerm.')

	if (len( sys.argv )==1) or ('-h' in sys.argv):
		show_help()
		exit(1)

	args = get_args( sys.argv ) # gets 'source', 'raw', 'hex'
	# print( args )
	ser = SerialHelper( args )

	# List of tests
	_names = get_test_names()
	print( "type test name to execute it or exit to quit!")
	print( "test names: %s" % ", ".join(_names) )
	_cmd = 'none'
	while _cmd != 'exit':
		_cmd = input("Which test? ")
		if _cmd in _names:
			ser.run_test( _cmd )
			print('') # add a spacer
	print( "That's all folks!" )
