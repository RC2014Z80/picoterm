/* ==========================================================================
                    Standard definition for Picoterm
   ========================================================================== */

#include "picoterm_stddef.h"
#include <string.h>

const char* get_font_face_name( uint8_t font_face ){
    // high nibble of font_id contains the font-face
    switch (font_face) {
        case FFACE_MONO8:
            return "Mono8";
        case FFACE_OLIVETTITHIN:
            return "OlivettiThin";
        default:
            return "?FFace?";
    }
}

const char* get_charset_name( uint8_t charset ){
    // low nibble of font_id contains the charset name
    switch (charset) {
        case CHARSET_NUPETSCII:
            return "NupetScii";
        case CHARSET_CP437:
            return "CP437";
        default:
            return "?charset?";
    }
}

const char* get_font_name( uint8_t font_id ){
    static char _msg[30];
    switch (font_id) {
        case FONT_ASCII: /* very spécial case */
            return "ASCII";
        default:
            sprintf( _msg, "%s-%s", get_font_face_name(font_id & 0xF0), get_charset_name(font_id & 0x0F) );
            return _msg;
    }
}

const bool has_charset( uint8_t font_id, uint8_t charset ){
    // Low Nibble of font_ID contains the charset, High nibble contains the Font-Face
    return (font_id & 0x0F) == charset;
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
