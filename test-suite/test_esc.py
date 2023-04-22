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
		self.ser = serial.Serial(args['device'], 115200, timeout=0.500 ) # 100ms

	def write_str( self, str ):
		""" Generic write that replaces \ESC and \0x1b in unicode string """
		self.ser.write( str.replace('\ESC',chr(27)).encode('ASCII') )

	def write_byte( self, val ):
		assert 0<= val <= 255, "Byte value %s out-of-scope" % val
		self.ser.write( bytes([val]) )

	def readline( self ):
		return self.ser.readline()

	def run_test( self, test_name ):
		""" Call a test function and display information locally and remotely """
		print( "Run: %s" % test_name )
		print( get_docstring( test_name ))
		ser.write_str( "Run: %s\r\n" % test_name )
		eval( 'test_%s(ser)' % test_name )

	def list_test( self, partial_name ):
		""" enumerate the test having the partial name in it + display its name and help """
		print( "List tests for: %s" % partial_name )
		_list = get_test_names()
		for _name in _list:
			if partial_name in _name:
				print( "--- %s --------------------------------" % _name )
				print( get_docstring( _name ))
		print( ' ' )

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
def test_ansi_charset( ser ):
	""" switch to ANSI graphical charset (NupetScii/CP437) and display full charset """
	ser.write_str( "\ESCF" )
	for row in range( 0x20, 0xFF, 0xF+1 ):
		ser.write_str( "%0x : " % row )
		for col in range( 0x0, 0x0F+1 ):
			ser.write_byte( row+col )
			ser.write_byte( 0x20 ) #space
		ser.write_byte( 13 )
		ser.write_byte( 10 )

def test_ansi_charset2( ser ):
	""" switch to ANSI graphical charset (NupetScii/CP437) and display a range for characters in normal, blink, reverse and return to normal """
	def draw_serie():
		for _char in range( 0xD0, 0xDF+1 ):
			ser.write_byte( _char )
			ser.write_byte( 0x20 ) #space
		ser.write_str( "\r\n\r\n" )

	ser.write_str( "\ESCF" ) # Switch to NuPetScii
	# Normal Drawing
	draw_serie()
	ser.write_str( "\ESC[5m" ) # Set Blinking
	draw_serie()
	ser.write_str( "\ESC[25m" ) # Reset Blinking
	# Inverted
	ser.write_str( "\ESC[7m" ) # inverted
	draw_serie()
	# ser.write_str( "\ESC[27m" ) # reset inverse/reverse mode

	ser.write_str( "\ESC[0m" ) # BAck to normal
	ser.write_str( "\r\nBack to normal" )


def test_ascii_charset( ser ):
	""" switch to ASCII (7bit) and display full charset """
	ser.write_str( "\ESCG" )
	for row in range( 0x20, 0xFF, 0xF+1 ):
		ser.write_str( "%0x : " % row )
		for col in range( 0x0, 0x0F+1 ):
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

def test_no_wrap( ser ):
	""" Disable WarpAround and send a Lorem Ipsum """
	ser.write_str( "\ESC[?7l" )
	test_lorem( ser )

def test_do_wrap( ser ):
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

def test_40col_char_insert( ser ):
	""" Send Lorem Ipsum, go first line at third character, insert 8 spaces """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[1;3H") # Line 2, Col 3
	time.sleep(3)
	ser.write_str("\ESC[8@") # Line 2, Col 3

def test_line_insert( ser ):
	""" Send Lorem Ipsum, go second line at third character, insert 1 line """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3H") # Line 2, Col 3
	time.sleep(3)
	ser.write_str("\ESC[1L") # Insert 1 Line

def test_line_delete( ser ):
	""" Send Lorem Ipsum, go second line at third character, delete 1 line """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3H") # Line 2, Col 3
	time.sleep(3)
	ser.write_str("\ESC[1M") # Delete 1 Line

def test_line_delete3( ser ):
	""" Send Lorem Ipsum, go second line at third character, delete 3 lines """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[2;3H") # Line 2, Col 3
	time.sleep(3)
	ser.write_str("\ESC[3M") # Delete 3 Lines

def test_blink( ser ):
	""" Display text with blinking parts and no blinking parts  """
	ser.write_str("This text should NOT BLINK!\r\n")
	ser.write_str("\ESC[5m") # Blink on
	ser.write_str("This text should be blinking!")
	ser.write_str("\ESC[25m") # Blink Off
	ser.write_str(" Except the end of the line.\r\n")
	ser.write_str("This text should NOT BLINK!\r\n")

def test_clear_to_cursor( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 5th char, clear up to cursor """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	ser.write_str("\ESC[1J") # Clear to cursor

def test_scroll_up( ser ):
	""" Send message + Lorem Ipsum, wait 5 sec, set cursor to 5th line & 5th char, move whole screen
	    up of 1 line. Cursor not move with scrolling."""
	test_clear(ser)
	ser.write_str( 'This first line should not be visible because\r\n' )
	ser.write_str( 'of scroll 1 lineup. BUT THIS LINE SHOULD STAY VISIBLE.\r\n')
	test_lorem(ser)
	ser.write_str( 'Wait 5 sec before scroll up.')
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	time.sleep(5)
	ser.write_str("\ESC[S") # Scroll whole screen 1 line up.

def test_scroll_up3( ser ):
	""" Send message + Lorem Ipsum, wait 5 sec, set cursor to 5th line & 5th char, move whole screen
	    up of 3 lines up. Cursor not move with scrolling."""
	test_clear(ser)
	ser.write_str( 'First line with "Run: xxx" should not be visible because\r\n' )
	ser.write_str( 'of scroll 1 lineup (THIS LINE SHOULD NOT BE VISIBLE).\r\n')
	ser.write_str( 'And this additionnal line LINE SHOULD NOT BE VISIBLE EITHER!.\r\n')
	test_lorem(ser)
	ser.write_str( 'Wait 5 sec before scroll up.')
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	time.sleep(5)
	ser.write_str("\ESC[3S") # Scroll whole screen 3 lines up.

def test_scroll_down( ser ):
	""" Send message + Lorem Ipsum, wait 5 sec, set cursor to 5th line & 5th char, move whole screen
	    down of 1 line. Cursor not move with scrolling."""
	test_clear(ser)
	ser.write_str( 'This is the first line of screen.\r\n' )
	test_lorem(ser)
	ser.write_str( 'Wait 5 sec before scroll down.')
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	time.sleep(5)
	ser.write_str("\ESC[T") # Scroll whole screen 1 line down.

def test_scroll_down3( ser ):
	""" Send message + Lorem Ipsum, wait 5 sec, set cursor to 5th line & 5th char, move whole screen
	    down of 3 lines up. Cursor not move with scrolling."""
	test_clear(ser)
	test_clear(ser)
	ser.write_str( 'This is the first line of screen.\r\n' )
	test_lorem(ser)
	ser.write_str( 'Wait 5 sec before scroll down.')
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	time.sleep(5)
	ser.write_str("\ESC[3T") # Scroll whole screen 3 line down.

def test_clear_to_eol( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 5th char, clear up from cursor to end-of-line."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;5H") # Line 5, Col 5
	ser.write_str("\ESC[0K") # clear from cursor to end of line

def test_clear_from_bol( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 50th char, clear up from begin of line to cursor."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;50H") # Line 5, Col 5
	ser.write_str("\ESC[1K") # clear from begin of line to cursor

def test_clear_line( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 50th char, clear the whole line."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;50H") # Line 5, Col 5
	ser.write_str("\ESC[2K") # clear the while line

def test_char_delete( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 20th char, delete 10 chars.
	The end-of-line should shift left of 10 chars from cursor (cursor position included)."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;20H") # Line 5, Col 20
	ser.write_str("\ESC[10P") # delete 10 characters

def test_40col_char_delete( ser ):
	""" Send Lorem Ipsum, set cursor to 3th line & 20th char, delete 10 chars.
	The end-of-line should shift left of 10 chars from cursor (cursor position included)."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[3;20H") # Line 3, Col 20
	time.sleep(3)
	ser.write_str("\ESC[10P") # delete 10 characters

def test_40col_char_erase( ser ):
	""" Send Lorem Ipsum, set cursor to 3th line & 20th char, Erase 10 chars.
	The char under the cursor and next 9 chars should be blank (10 chars cursor position included)."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[3;20H") # Line 3, Col 20
	time.sleep(3)
	ser.write_str("\ESC[10X") # erase 10 characters

def test_char_delete80( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 80th char, delete 10 chars.
	The end-of-line should shift left of 10 chars from cursor. Side effect test, only position 80 must be cleared."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;80H") # Line 5, Col 5
	ser.write_str("\ESC[10P") # delete 10 characters

def test_char_erase( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 50th char, Erase 10 chars on the right of cursor (cursor position included).
	The erased chars are replaced with space."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;50H") # Line 5, Col 5
	ser.write_str("\ESC[10X") # erase 10 characters

def test_char_erase80( ser ):
	""" Send Lorem Ipsum, set cursor to 5th line & 80th char, Erase 10 chars on the right of cursor (cursor position included).
	The erased chars are replaced with space. Side effect test, only position 80 must be cleared."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[5;80H") # Line 5, Col 5
	ser.write_str("\ESC[10X") # erase 10 characters

def test_cursor_move( ser ):
	""" Place a X at 10, 10. Move back cursor at 10, 10 (movement center). Wait 2 sec, Move up 5 char, wait 2, return to center. Repeat movement for right, down, left. """
	test_clear(ser)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	ser.write_str("X")
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[5A") # Move up 5 char
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[5C") # Move right/forward 5 char
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[5B") # Move down 5 char
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[5D") # Move left/backward 5 char
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

def test_clear_eos_vt52( ser ):
	""" vt52: Lorem ipsum, go home, move 2 lines down+1 char right, clear until the end of screen """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[?2l") # enter vt52 mode
	ser.write_str("\ESCH") # go home
	ser.write_str("\ESCB\ESCB\ESCC") # enter vt52 mode
	ser.write_str("\ESCJ") # Clear end of screen
	ser.write_str("\ESC<") # enter vt100 mode

def test_clear_eol_vt52( ser ):
	""" vt52: Lorem ipsum, go home, move 2 lines down+1 char right, clear until the end of line """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[?2l") # enter vt52 mode
	ser.write_str("\ESCH") # go home
	ser.write_str("\ESCB\ESCB\ESCC") # enter vt52 mode
	ser.write_str("\ESCK") # Clear end of line
	ser.write_str("\ESC<") # enter vt100 mode

def test_cursor_move_vt52( ser ):
	""" vt52. Place a X at 10, 10. Move back cursor at 10, 10 (movement center). Wait 2 sec, Move up 3 char, wait 2, return to center. Repeat movement for right, down, left. """
	test_clear(ser)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	ser.write_str("X")
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[?2l") # enter vt52 mode
	for i in range(3):
		ser.write_str("\ESCA") # Move up 3 char
	ser.write_str("\ESC<") # enter vt100 mode
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[?2l") # enter vt52 mode
	for i in range(3):
		ser.write_str("\ESCC") # Move right/forward 3 char
	ser.write_str("\ESC<") # enter vt100 mode
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[?2l") # enter vt52 mode
	for i in range(3):
		ser.write_str("\ESCB") # Move down 3 char
	ser.write_str("\ESC<") # enter vt100 mode
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

	ser.write_str("\ESC[?2l") # enter vt52 mode
	for i in range(3):
		ser.write_str("\ESCD") # Move left/backward 3 char
	ser.write_str("\ESC<") # enter vt100 mode
	time.sleep(2)
	ser.write_str("\ESC[10;10H") # Line 10, Col 10
	time.sleep(2)

def test_home_vt52( ser ):
	""" vt52: Send Lorem Ipsum. move to 0-0 (home) """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[?2l") # enter vt52 mode
	ser.write_str("\ESCH") # go Home
	ser.write_str("\ESC<") # enter vt100 mode

def test_cursor_at_line( ser ):
	""" Send Lorem Ipsum, place cursor to 3th line & 5th char, move at absolute line 10. Cursor still at 5th char."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[3;5H") # Line 3, Col 5
	ser.write_str("\ESC[10d") # Move absolute line 10

def test_cursor_at_col( ser ):
	""" Send Lorem Ipsum, place cursor to 3th line & 5th char, move at absolute column 13."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[3;5H") # Line 3, Col 5
	ser.write_str("\ESC[13G") # Move absolute column 13

def test_cursor_down_bol( ser ):
	""" Send Lorem Ipsum, place cursor to 3th line & 5th char, move cursor 6 lines down (at begin of line). Cursor must be on line 9. """
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[3;5H") # Line 3, Col 5
	ser.write_str("\ESC[6E") # move 6 line down at begin-of-line

def test_cursor_up_bol( ser ):
	""" Send Lorem Ipsum, place cursor to 10th line & 5th char, move cursor 2 lines up (at begin of line). Cursor must be at line 8."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str("\ESC[10;5H") # Line 3, Col 5
	time.sleep(2)
	ser.write_str("\ESC[2F") # move 2 lines up at begin-of-line

def test_reverse( ser ):
	""" Send Lorem Ipsum, set reverse, write some text, reset reverse, write some text."""
	test_clear(ser)
	test_lorem(ser)
	ser.write_str( "\ESC[7m" ) # reverse text
	ser.write_str( "This text should be displayed in reverse.\r\n" )
	ser.write_str( "\ESC[27m" ) # reset inverse/reverse mode
	ser.write_str( "But this line should not be displayed in Reverse\r\n")

def test_back_to_normal( ser ):
	""" Write some inverted text, some blink text, some inverted+blink. Finally, return to normal text."""
	test_clear(ser)
	ser.write_str( "\ESC[7m" ) # inverted
	ser.write_str("some inverted text.\r\n")
	ser.write_str( "\ESC[27m" ) # reset inverse/reverse mode
	ser.write_str( "\ESC[5m" ) # Blink ON
	ser.write_str("some Blinking text.\r\n")
	ser.write_str( "\ESC[25m" ) # Blink OFF

	ser.write_str( "\ESC[7m" ) # inverted
	ser.write_str( "\ESC[5m" ) # Blink ON
	ser.write_str( "some Blinking Inverted text.\r\n" )

	ser.write_str( "\ESC[0m" ) # Back to normal
	ser.write_str( "This must be back to normal :-)" )

def test_cursor_style( ser ):
	""" cycle throught the 6 type of cursor. Display its name and wait 5 sec for each."""
	cursors = [ ( "\ESC[1 q", "Blinking block cursor shape"),
				( "\ESC[2 q", "Steady block cursor shape"),
				( "\ESC[3 q", "Blinking underline cursor shape"),
				( "\ESC[4 q", "Steady underline cursor shape"),
				( "\ESC[5 q", "Blinking bar cursor shape"),
				( "\ESC[6 q", "Steady bar cursor shape"),
				( "\ESC[0 q", "Default cursor shape configured by the user ") ]
	test_clear(ser)
	for _esc, _label in cursors:
		ser.write_str("\r\n")
		ser.write_str( "%s then wait 5 sec." % _label )
		ser.write_str( _esc )
		time.sleep( 5 )
	ser.write_str( "\r\nDone!" )

def test_dec_lines( ser, switch_ansi=True ):
	""" Switch to Graphical ANSI font, Draw DEC lines (simple, double and normal chars). """
	entries = ( 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x71, 0x74, 0x75, 0x76, 0x77, 0x78 )

	def draw_serie():
		for _c in entries:
			ser.write_byte( _c )
			ser.write_byte( 0x20 ) # Space

	if switch_ansi:
		ser.write_str( "\ESCF" ) # Switch to graphic mode
		ser.write_str( "We are in Graphic ANSI mode\r\n")
	else:
		ser.write_str( "\ESCG" ) # Switch to graphic mode
		ser.write_str( "Ensure we are ASCII mode\r\n")

	ser.write_str( "We switch in simple DEC line mode\r\n")
	ser.write_str( "\ESC(0" )
	draw_serie()
	ser.write_str( "\ESC(B" ) # we exit DEC line mode
	ser.write_str( "\r\n" )
	ser.write_str( "We switch in double DEC line mode\r\n")
	ser.write_str( "\ESC(2" )
	draw_serie()
	ser.write_str( "\ESC(B" ) # we exit DEC line mode

	if switch_ansi:
		ser.write_str( "\r\n" )

		ser.write_str( "We are now exit DEC Line Drawing.\r\n")
		ser.write_str( "Graphic ANSI mode is still active (NupetScii/cp437).\r\n")
	else:
		ser.write_str( "\r\n" )
		ser.write_str( "We never exit ASCII (but exited the DEC Line Drawing)\r\n")
		ser.write_str( "ASCII mode is still active.\r\n")

	ser.write_str( "Anyway we should read this entire line.\r\n\r\n")
	return

def test_dec_lines2( ser, switch_ansi=True ):
	""" Switch to ASCII, Draw DEC lines (simple, double and normal chars). """
	test_dec_lines( ser, switch_ansi=False )


def test_vt100_status( ser ):
	""" Request the VT100ID (two different way) and display it. Ask VT100 Status and display it"""
	ser.write_str( "\ESC[0c" ) # Ask VT100ID
	data = ser.readline() # rely on the CR time-out for the end of reading
	if (data != None) and len(data)>0 :
		print( "VT100ID : \ESC[0c response: \ESC%s  (expecting something like \ESC[?1;0c , where 1 is the is ID)" % data[1:].decode("ASCII") )
	else:
		print( "VT100ID : Oups! no response!  (expecting something like \ESC[?1;0c , where 1 is the is ID)")

	ser.write_str( "\ESC[c" ) # Ask VT100ID
	data = ser.readline() # rely on the CR time-out for the end of reading
	if (data != None) and len(data)>0 :
		print( "VT100ID : \ESC[c response: \ESC%s  (expecting something like \ESC[?1;0c , where 1 is the is ID)" % data[1:].decode("ASCII") )
	else:
		print( "VT100ID : Oups! no response!  (expecting something like \ESC[?1;0c , where 1 is the is ID)")

	ser.write_str( "\ESC[5n" ) # Ask VT100 status
	data = ser.readline() # rely on the CR time-out for the end of reading
	if (data != None) and len(data)>0 :
		print( "VT100 status : \ESC[5n response: \ESC%s  (expecting something like \ESC[0n ; where 0 is the is the status)" % data[1:].decode("ASCII") )
	else:
		print( "VT100 status : Oups! no response!  (expecting something like \ESC[0n , where 0 is the is the status)")

def test_term_id_vt52( ser ):
	""" vt52: Request the Terminal ID and display it """
	ser.write_str("\ESC[?2l") # enter vt52 mode
	ser.write_str( "\ESCZ" ) # Ask Term_ID
	data = ser.readline() # rely on the CR time-out for the end of reading
	if (data != None) and len(data)>0 :
		print( "TerminalID : \ESCZ response: \ESC%s  (expecting something like ??????????)" % data[1:].decode("ASCII") )
	else:
		print( "TerminalID : Oups! no response!  (expecting something like ???????)")
	ser.write_str("\ESC<") # enter vt100 mode
	print( data ) # debug

def test_term_id2_vt52( ser ):
	""" vt52: Request the Terminal ID and display it """
	ser.write_str("\ESC[?2l") # enter vt52 mode
	ser.write_str( "\ESC[Z" ) # Ask Term_ID
	data = ser.readline() # rely on the CR time-out for the end of reading
	if (data != None) and len(data)>0 :
		print( "TerminalID : \ESC[Z response: \ESC%s  (expecting something like \ESC[/Z)" % data[1:].decode("ASCII") )
	else:
		print( "TerminalID : Oups! no response!  (expecting something like \ESC[/Z)")
	ser.write_str("\ESC<") # enter vt100 mode
	print( data ) # debug

def test_reverse_lf_vt52( ser ):
	""" vt52: Send Lorem, set cursor at line 3, char 5, wait 2sec, reverse line feed. Result: cursor should move at line 2. """
	test_clear( ser )
	test_lorem( ser )
	ser.write_str("\ESC[3;5H") # Line 3, Col 5
	ser.write_str("\ESC[?2l") # enter vt52 mode
	time.sleep( 2 )
	ser.write_str( "\ESCI" )  # reverse LF
	ser.write_str("\ESC<") # enter vt100 mode

def test_reset_settings( ser ):
	""" Write some reverse text, write some blink text. Wait 3 seconds. send reset setting,
	send additional text. Result: text should be in plain text on a cleared screen. """
	test_clear( ser )
	ser.write_str( "\ESC[7m" ) # inverted
	ser.write_str("some inverted text.\r\n")
	ser.write_str( "\ESC[5m" ) # Blink ON
	ser.write_str("some Blinking inverted text.\r\n")
	time.sleep( 3 )
	ser.write_str( "\ESCc" ) # Reset setting (including screen)
	for i in range(10):
		ser.write_str( "This text should be plain display %s / 10\r\n" % (i+1) )

def test_reset( ser ):
	"""Just reset the terminal (not really a test, just an helper). """
	ser.write_str( "\ESCc" ) # Reset setting (including screen)

def screen_save( ser, seq_id ):
	test_clear(ser)
	ser.write_str( "\ESC[7m" ) # inverted
	ser.write_str("some inverted text.\r\n")
	ser.write_str( "\ESC[27m" ) # reset inverse/reverse mode
	ser.write_str( "\ESC[5m" ) # Blink ON
	ser.write_str("some Blinking text.\r\n")
	ser.write_str( "\ESC[25m" ) # Blink OFF

	ser.write_str( "\ESC[7m" ) # inverted
	ser.write_str( "\ESC[5m" ) # Blink ON
	ser.write_str( "some Blinking Inverted text.\r\n" )

	ser.write_str( "\ESC[0m" ) # Back to normal
	ser.write_str( "This must be back to normal :-)\r\n" )
	ser.write_str( "Wait 5 second before save and change screen\r\n" )
	time.sleep( 5 )

	ser.write_str( "\ESC[?%ih" % seq_id ) # Save screen

	test_clear( ser )
	for row in range( 0x20, 0xFF, 0xF+1 ):
		ser.write_str( "%0x : " % row )
		for col in range( 0x0, 0x0F ):
			ser.write_byte( row+col )
			ser.write_byte( 0x20 ) #space
		ser.write_byte( 13 )
		ser.write_byte( 10 )

	ser.write_str( "Wait 5 second before restoring the screen\r\n" )
	time.sleep(5)

	ser.write_str( "\ESC[?%il" % seq_id ) # Restore the screen

def test_screen_save( ser ):
	""" Write some text, save the screen. Display,	Ascii table, wait 5 sec
	then restore initial screen """
	screen_save( ser, seq_id=47 )

def test_screen_save_1047( ser ):
	""" Linux version with 1047: Write some text, save the screen. Display,	Ascii table, wait 5 sec
	then restore initial screen """
	screen_save( ser, seq_id=1047 )

def test_screen_save_1049( ser ):
	""" Linux version with 1049: Write some text, save the screen. Display,	Ascii table, wait 5 sec
	then restore initial screen """
	screen_save( ser, seq_id=1049 )


if __name__ == '__main__':
	print( '== PicoTerm Escape Sequence tester %s ==' % __version__ )
	print( 'Series of test to call on demand to check specific escape ')
	print( 'sequence handling on PicoTerm.')
	print( '' )


	if (len( sys.argv )==1) or ('-h' in sys.argv):
		show_help()
		exit(1)

	args = get_args( sys.argv ) # gets 'source', 'raw', 'hex'
	# print( args )
	ser = SerialHelper( args )

	# List of tests
	_names = get_test_names()
	print( '* Key-in "test_name" to execute it.' )
	print( '* Key-in "PARTIAL_test_name" will display name containing the string' )
	print( '* Key-in "?PARTTIAL test_name" will display name and HELP' )
	print( '* Key-in "." executes the last test_name again.')
	print( '-'*79 )
	print( "test names: %s" % ", ".join(_names) )
	print( '-'*79 )
	print( ' ' )
	_cmd = 'none'
	_last_cmd = 'none'
	while _cmd != 'exit':
		_cmd = input("Which test? ")
		if _cmd in _names:
			ser.run_test( _cmd )
			_last_cmd = _cmd
			print('') # add a spacer
		elif len(_cmd)==0:
			pass
		elif _cmd=="exit":
			pass
		elif _cmd==".":
			if _last_cmd in _names:
				ser.run_test( _last_cmd )
		elif _cmd[0] == '?':
			ser.list_test( _cmd[1:] )
		else:
			print("UNKNOWN COMMAND %s !" % _cmd )
			if len(_cmd)>0:
				print( "'%s' could match: %s" % (_cmd,", ".join( [_name for _name in _names if _cmd.upper() in _name.upper()])) )
	print( "That's all folks!" )
