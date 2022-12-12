#!/usr/bin/env python3
import sys
import os
import time
import json
import io
#import glob

__version__ = '0.1'

def show_help():
	print( '== PlayScii extractor %s ==' % __version__ )
	print( 'Extract RC2014 data from art/screen drawed with playscii')
	print( '                  by Meurisse D.')
	print( 'USAGE:')
	print( '  ./psci-extract.py <filename.psci> -r -h' )
	print( '' )
	print( '<filename>  : the PlayScii saved file' )
	print( '-r          : Create the .raw binary file (see description here below).')
	print( '-hex <addr> : Create an Intel Hex file to store the raw data from ')
	print( '              the given address (integer or hexadecimal like 0x8100).')
	print( '-h          : display this help.')
	print( '' )
	print( '=== .raw format ===' )
	print( 'W : 1 byte, column Width' )
	print( 'H : 1 byte, row Height' )
	print( 'Data: column*row bytes, Art/Screen data from row[1] to row[H]. Each row having W bytes (1 byte per column)')
	print( '')

def get_args( argv ):
	""" Process argv and extract: output, source, device, user parameters """
	r = { 'source' : None, 'raw' : None, 'hex' : None } # unamed, -r, -hex 0x0000
	used = [] # list of used entries in argv
	used.append(0) # item #0 is the script name
	unamed = [] # unamed parameters
	# Locate the named parameter
	for i in range( len(argv) ):
		if argv[i] == '-hex':
			sAddr = argv[i+1]
			try:
				if '0x' in sAddr:
					r['hex']=eval(sAddr)
				else:
					r['hex']=int(sAddr)
			except:
				raise Exception('Invalid interger/hex value for address' % sAddr )
			used.append(i)
			used.append(i+1)
		elif argv[i] == '-r':
			r['raw'] = True
			used.append(i)

	# Locate the unamed parameter
	for i in range( len(argv) ):
		if i in used:
			continue
		else:
			unamed.append( argv[i] )
	# First unamed is the source file
	if len(unamed) > 0:
		r['source'] = unamed[0]


	# Sanity check
	if r['source']==None:
		raise Exception('source filename is missing')

	if not( '.psci' in r['source'] ):
		raise Exception('source file must be .psci')

	if r['raw']==None and r['hex']==None: # Other mode should comes later
		raise Exception('No output mode selected!')

	return r

def extract_raw( filename ):
	""" compose the Raw data and returns a BytesIO stream """
	with open( filename ) as f:
		data = json.load( f )
	charset = data['charset']
	palette = data['palette']
	height  = data['height']
	width   = data['width']
	print( 'charset   : %s %s' % (charset, '*** WARNING *** not nupetscii !' if charset != 'nupetscii' else '') )
	print( 'palette   : %s' % palette )
	print( 'size      : %i x %i' % (width,height))
	screen = data['frames'][0]['layers'][0]['tiles']
	screen_name = data['frames'][0]['layers'][0]['name']
	print( 'layer name: %s' % (screen_name) )
	_r = io.BytesIO()
	# write the size
	_r.write( bytes([width,height]) )
	for entry in screen:
		_r.write( bytes([ entry['char']+0x20 ]) ) # char index 0 = espace = $20
	print( 'data size : %i bytes' % (_r.seek(0,2)))
	return _r

def create_raw_file( args ):
	filename = args['source']
	print( '%s to raw file' % (os.path.basename(filename)))
	print( 'source    : %s' % filename )
	destin = args['source'].replace( '.psci', '.raw' )
	stream = extract_raw( filename )
	#size = stream.seek( 0, 2 ) # seek at 0 bytes from end of file
	stream.seek( 0 )
	with open( destin, 'wb' ) as f:
		f.write( stream.getbuffer() )
	print( 'file created @ %s' % destin )

def create_hex_file( args ):
	try:
		from intelhex import IntelHex
	except:
		raise Exception( 'IntelHex library not available!')
	filename = args['source']
	print( '%s to RC20214 SCM hex' % (os.path.basename(filename)))
	print( 'source    : %s' % filename )
	destin = args['source'].replace( '.psci', '.hex' )
	stream = extract_raw( filename )
	size = stream.seek( 0, 2 ) # seek at 0 bytes from end of file
	print( 'target addr: %s' % args['hex'] )

	ih = IntelHex()
	stream.seek( 0 )
	ih.loadbin( stream, offset=args['hex'] )
	ih.write_start_addr = args['hex']
	ih.byte_count = 64
	ih.tofile( destin, format='hex' )
	print( 'file created @ %s' % destin )


if __name__ == '__main__':
	if (len( sys.argv )==1) or ('-h' in sys.argv):
		show_help()
		exit(1)

	args = get_args( sys.argv ) # gets 'source', 'raw', 'hex'
	#print( 'Number of arguments: %i' % len(sys.argv) )
	#print( 'Argument List: %s' % sys.argv )
	#print( 'Decoded args: %s' % args )
	if args['raw'] != None:
		create_raw_file( args )

	if args['hex'] != None:
		create_hex_file( args )
