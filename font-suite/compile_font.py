#!/usr/bin/env python
""" compile_font.py - generating font8-ext.C file

	read the font8.c and inject the font8-ext.data into it.
"""

import datetime

def get_c_def( s, name, default='' ):
	# extract a value from a c_def string like the following
	#    {.bitmap_index = @bitmap_index@, .adv_w = 128, .box_h = 4, .box_w = 4, .ofs_x = 2, .ofs_y = 2}
	s = s.strip()
	if not( ('.%s'%name) in s):
		return default
	_l = s.replace('{','').replace('}','').split(',')
	for _item in _l:
		if ('.%s'%name) in _item:
			return _item.split('=')[1].strip()
	return default

class FontExtData:
	""" remarks: contains the remark line. Eg: /* U+80 box (129) */
		c_def: contains the c definition. eg: {.bitmap_index = @bitmap_index@, ... }
		data: contains the list of bytes compiled from encoded graphic (4bpp coding) """

	__slots__ = ("remark","c_def","data","__4bits" )

	def __init__(self):
		self.remark = ""
		self.c_def = ""
		self.data = []
		self.__4bits = None

	def add_4bits( self, value ):
		if self.__4bits == None:
			self.__4bits = value
		else:
			self.data.append( (self.__4bits << 4) + value )
			self.__4bits = None

	def extract_c_def( self, name ):
		_r = get_c_def( self.c_def, name, None )
		if _r==None:
			raise Exception( 'No c_def for %s' % name )
		return _r

	@property
	def has_4bits( self ):
		return self.__4bits != None

	def __repr__( self ):
		return "<FontExtData \"%s\">" % self.remark.replace('/*','').replace('*/','').strip()

	def print_it( self ):
		def split_str(seq, chunk, skip_tail=False):
			lst = []
			if chunk <= len(seq):
				lst.extend([seq[:chunk]])
				lst.extend(split_str(seq[chunk:], chunk, skip_tail))
			elif not skip_tail and seq:
				lst.extend([seq])
			return lst

		def as_bin( i ):
			# Transform a value into 8bits string
			s = bin( i ).replace( '0b', '' )
			return ('%8s' % s).replace(' ','0')

		print( self ) # show representation
		s = ''
		for i in self.data:
			s += as_bin( i )

		# 4 Bit per Pixel
		bpp_list = split_str( s, 4 )
		BPP_AS_CHAR = [' ', ' ', ' ', '.', '.', '.', '.', '.', '.', '.', '.', '.', '*', '*', '*', '*']

		_s = ''
		for entry in bpp_list:
			_s += BPP_AS_CHAR[ eval('0b%s' % entry) ]
		box_w = int( self.extract_c_def('box_w') )
		for substr in split_str( _s, box_w ):
			print( substr )
		print( '' )

def load_extensions( filename ):
	""" parse the '-ext.txt' file and return a list of FontExtData """
	print( 'Loading extension %s' % filename )
	_l = []
	with open( filename, 'r' ) as f:
		lines = f.readlines()

	_ext_data = None
	for line in lines:
		line = line.replace('\r','').replace('\n','')
		if ('/*' in line) and ('*/' in line):
			# be sure that we pushed every 4bpp to the data list.
			if (_ext_data != None) and (_ext_data.has_4bits!=None):
				_ext_data.add_4bits( 0x00 )
			# Add a new entry
			_ext_data = FontExtData()
			_l.append( _ext_data )
			_ext_data.remark = line.strip()
		elif ('.bitmap_index' in line ):
			_ext_data.c_def = line.strip()
		else:
			# It is a string with F....F defining entry
			try:
				for c in line.strip():
					if c == '.':
						_ext_data.add_4bits( 0x0 )
					else:
						_ext_data.add_4bits( eval('0x%s'%c) )
			except Exception as err:
				_txt = ""
				if _ext_data.remark != None:
					_txt = _ext_data.remark
				raise Exception( "coding error for %s with %s %s" % (_txt, err.__class__, err ) )

	# Sanity Check - box_h et box_w against data encoded
	for _ext in _l:
		_box_h = int( _ext.extract_c_def('box_h') )
		_box_w = int( _ext.extract_c_def('box_w') )
		if len(_ext.data) != (_box_h * _box_w)//2:
			raise Exception( "Invalid box_h %i * box_w %i for %s. It doesn't fit the data size %i!" % (_box_h, _box_w, _ext.remark, len(_ext.data)) )
	return _l

def inject_extension( font_source, font_destin, exts, subs ):
	""" font_source : source filename (usually font8.c).
	    font_destin : destination file (eg: cp431.c).
		ext : the collection of extension to be injected in the destination file.
		subs : a list of tuples ( source_string, replacement string ). """

	def substitute( line ):
		for s,d in subs:
			if s in line:
				return line.replace(s,d)
		return line

	_r = []
	_r.append( '/* generated with compile_font.py on %s */\r\n' % datetime.datetime.now().strftime("%B %d, %Y  %H:%M:%S" ) )

	with open( font_source ) as f:
		_l = f.readlines()
	# Looking for the "};" after the "/* U+7E "~" */"
	while True:
		line = _l.pop(0)
		if not( "};" in line ):
			_r.append( substitute( line ) )
		else:
			break

	_r.append( '\r\n' )
	_r.append( '/* FONT EXTENSION  added by compile_font.py */ \r\n')
	_r.append( '\r\n' )

	# Insert the extension data
	for ext in exts:
		_r.append( ext.remark )
		_r.append( '\r\n' )
		_r.append( ',' )
		_r.append( ','.join( [hex(value) for value in ext.data]  ) )
		_r.append( '\r\n' )

	# Append the structure closure (we already read)
	_r.append( substitute( line ) )

	# Locate start of Glyph description section
	while True:
		line = _l.pop(0)
		if 'lv_font_fmt_txt_glyph_dsc_t' in line:
			_r.append( substitute(line) )
			break
		_r.append( substitute(line) )
	# from now, we capture the bitmap_index value while copying the line
	# to the destination
	last_index = 0
	last_box_h = 0
	lasy_box_w = 0
	while True:
		line = _l.pop(0)
		# detect the end of Glyph structure
		if "};" in line:
			break
		_r.append( substitute(line) )
		# capture the last "bitmap_index" value
		if '.bitmap_index' in line:
			last_index = int( get_c_def( line, 'bitmap_index', -1 ) )
			last_box_h = int( get_c_def( line, 'box_h', -1 ) )
			last_box_w = int( get_c_def( line, 'box_w', -1 ) )

	print( 'last bitmap_index = %i, box_h = %i, box_w = %i' % (last_index, last_box_h, last_box_w) )

	# now we can add our extension declaration :-)
	_r.append( '\r\n' )
	_r.append( '/* FONT EXTENSION added by compile_font.py */ \r\n')
	_r.append( '\r\n' )
	for ext in exts:
		last_index = last_index + ((last_box_h*last_box_w)/2)
		_r.append( ',%s\r\n' % ext.c_def.replace( '@bitmap_index@', str(last_index)) )
		last_box_h = int(ext.extract_c_def( 'box_h' ))
		last_box_w = int(ext.extract_c_def( 'box_w' ))
		if (last_box_h*last_box_w)%2 !=0:
			raise Exception( "Error on %s : box_h * box_w is not a EVEN number!" % ext.remark )
	# Close the data structure
	_r.append( substitute(line) )

	# Locate the lv_font_fmt_txt_cmap_t declaration
	# we must change the value of .range_length
	while True:
		line = _l.pop(0)
		if 'range_length' in line:
			break
		_r.append( substitute(line) )
	# Now update the line to add the extra items we have
	_new_items = []
	_items = line.split(',')
	for _item in _items:
		if 'range_length' in _item:
			_new_items.append( '%s+%s' % (_item, len(exts)) )
		else:
			_new_items.append( _item )
	line = ','.join(_new_items) # recompose the line
	# write the updated line
	_r.append( substitute(line) )

	# We finish the source file copy
	while len(_l)>0:
		line = _l.pop(0)
		if line == None:
			break
		_r.append( substitute(line) )

	# save the destination file
	with open( font_destin, 'w' ) as f:
		for line in _r:
			f.write( line )


if __name__ == '__main__':
	#for item in exts:
	#	item.print_it()
	exts = load_extensions( 'nupetscii.data' )
	inject_extension( 'font8.c', 'mono8_nupetscii.c', exts,
	  [ ("#ifndef UBUNTU_MONO" , "#ifndef NUPETSCII"),
	    ("#define UBUNTU_MONO" , "#define NUPETSCII"),
		("const lv_font_t ubuntu_mono8 = {", "const lv_font_t nupetscii_mono8 = {"),
		("#endif /*#if UBUNTU_MONO*/", "#endif /*#if NUPETSCII*/") ] )
	print( 'mono8_nupetscii.c created!')
	inject_extension( 'olivetti_thin.c', 'olivetti_thin_nupetscii.c', exts,
	  [ ("#ifndef OLIVETTI_THIN" , "#ifndef OLIVETTI_THIN_NUPETSCII"),
	    ("#define OLIVETTI_THIN" , "#define OLIVETTI_THIN_NUPETSCII"),
		("const lv_font_t olivetti_thin = {", "const lv_font_t nupetscii_olivetti_thin = {"),
		("#endif /*#if OLIVETTI_THIN*/", "#endif /*#if OLIVETTI_THIN_NUPETSCII*/") ] )
	print( 'olivetti_thin_nupetscii.c created!')

	exts = load_extensions( 'cp437.data' )
	inject_extension( 'font8.c', 'mono8_cp437.c', exts,
	  [ ("#ifndef UBUNTU_MONO" , "#ifndef CP437"),
	    ("#define UBUNTU_MONO" , "#define CP437"),
		("const lv_font_t ubuntu_mono8 = {", "const lv_font_t cp437_mono8 = {"),
		("#endif /*#if UBUNTU_MONO*/", "#endif /*#if CP437*/") ] )
	print( 'mono8_cp437.c created!')
	inject_extension( 'olivetti_thin.c', 'olivetti_thin_cp437.c', exts,
	  [ ("#ifndef OLIVETTI_THIN" , "#ifndef OLIVETTI_THIN_CP437"),
	    ("#define OLIVETTI_THIN" , "#define OLIVETTI_THIN_CP437"),
		("const lv_font_t olivetti_thin = {", "const lv_font_t cp437_olivetti_thin = {"),
		("#endif /*#if OLIVETTI_THIN*/", "#endif /*#if OLIVETTI_THIN_CP437*/") ] )
	print( 'olivetti_thin_cp437.c created!')
