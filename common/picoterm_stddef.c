/* ==========================================================================
                    Standard definition for Picoterm
   ========================================================================== */

#include "picoterm_stddef.h"
#include <string.h>

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


int replace_char(char *str, char orig, char rep) {
    char *ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}
