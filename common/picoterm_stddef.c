/* ==========================================================================
                    Standard definition for Picoterm
   ========================================================================== */

#include "picoterm_stddef.h"

const char* get_font_name( uint8_t font_id ){
	switch (font_id) {
		case FONT_ASCII:
			return "ASCII";
		case FONT_NUPETSCII:
			return "NuPetSCII";
		case FONT_CP437:
			return "CP437";
		default:
			return "???";
	}
}
