/* ==========================================================================
           Manage the cursor Appearance/pisition for the PicoTerm
   ========================================================================== */

#include "picoterm_dec.h"
#include "picoterm_stddef.h"

//uint8_t dec_mode = DEC_MODE_NONE; // single or double line drawing


char get_dec_char( uint8_t font_id, uint8_t dec_mode, char _char ){
	 // return the ASCII char for a given type of line drawing
	 if( !(_char >= 'j' && _char <= 'x') )
		return _char;
	char _r = 0x20;
	switch( font_id ){
		case FONT_NUPETSCII_MONO8:
				switch(_char){
					case 'j': // ┘
						_r =  dec_mode == DEC_MODE_SINGLE_LINE ? 0xBD : 0xE7;
						break;
					case 'k': // ┐
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xAE : 0xE4;
						break;
					case 'l': // ┌
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB0 : 0xE2;
						break;
					case 'm': // └
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xAD : 0xE5;
						break;
					case 'n': // ┼
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xDB : 0xEA;
						break;
					case 'q': // ─
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC3 : 0xE1;
						break;
					case 't': // ├
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xAB : 0xE8;
						break;
					case 'u': // ┤
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB3 : 0xE9;
						break;
					case 'v': // ┴
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB1 : 0xE6;
						break;
					case 'w': // ┬
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB2 : 0xE3;
						break;
					case 'x': // │
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xDD : 0xE0;
						break;
					default:
						_r = 0x5E; // ^ because DEC line code undefined!
						break;
				} // switch(_char)
				break;

		case FONT_CP437_MONO8:
				switch(_char){
					case 'j': // ┘
						_r =  dec_mode == DEC_MODE_SINGLE_LINE ? 0xD9 : 0xBC;
						break;
					case 'k': // ┐
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xBF : 0xBB;
						break;
					case 'l': // ┌
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xDA : 0xC9;
						break;
					case 'm': // └
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC0 : 0xC8;
						break;
					case 'n': // ┼
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC5 : 0xCE;
						break;
					case 'q': // ─
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC4 : 0xCD;
						break;
					case 't': // ├
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC3 : 0xCC;
						break;
					case 'u': // ┤
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB4 : 0xB9;
						break;
					case 'v': // ┴
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC1 : 0xCA;
						break;
					case 'w': // ┬
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xC2 : 0xCB;
						break;
					case 'x': // │
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0xB3 : 0xBA;
						break;
					default:
						_r = 0x5E; // ^ because DEC line code undefined!
						break;
				} // switch(_char)
				break;

		case FONT_ASCII:
				switch(_char){
					case 'j': // ┘
					case 'k': // ┐
					case 'l': // ┌
					case 'm': // └
					case 'n': // ┼
					case 't': // ├
					case 'u': // ┤
					case 'v': // ┴
					case 'w': // ┬
						_r =  dec_mode == DEC_MODE_SINGLE_LINE ? '+' : 0x8A; // +, inverted +
						break;
					case 'q': // ─
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? '=' : 0x9C; // -, inverted -
						break;
					case 'x': // │
						_r = dec_mode == DEC_MODE_SINGLE_LINE ? 0x7C : 0xDB; // pipe, inverted pipe
						break;
					default:
						_r = 0x5E; // ^ because DEC line code undefined!
						break;
				} // switch(_char)
				break;


		default: // UNSUPPORTED font_id
			// The # is used to identify this use-case. Do not modify it, just made
			// a proper implementation in the switch case.
			_r = 0x23;
	} // switch( font_id )

	return _r;
}
