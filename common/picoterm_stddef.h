#ifndef _PICOTERM_STDDEF_H_
#define _PICOTERM_STDDEF_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/scanvideo.h"

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})



/* ==========================================================================
                   Standard definition for Picoterm
   ========================================================================== */

static uint16_t palette[] = {
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0),        //black
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0, 0),     // red
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0xAA, 0),     //green
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0x55, 0),  //brown
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0xAA),     //blue
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0, 0xAA),  // magenta
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0xAA, 0xAA),  // cyan
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0xAA, 0xAA),//light grey
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0x55, 0x55),//grey
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x55, 0x55),//bright red
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0xFF, 0x55),//bright green
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0x55),//yellow
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0xFF),      //bright blue
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x55, 0xFF),//bright magenta
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0xFF, 0xFF),//bright cyan
      PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0xFF),//white
};

// Font Face - High Nibble 0x00, 0x10, 0x20 ... (up to 16 font face)
#define FFACE_MONO8  0x00
#define FFACE_OLIVETTITHIN 0x10

// Charset - Low Nibble 0x01, 0x02, 0x03 ... (up to 16 charset)
#define CHARSET_NUPETSCII 0x01
#define CHARSET_CP437     0x02

// FONT_ID
#define FONT_ASCII     0 /* 7 bits fonts (8th bit do reverse the char) */
#define FONT_NUPETSCII_MONO8        CHARSET_NUPETSCII + FFACE_MONO8 /* ANSI is 8 bits , = 0x01 */
#define FONT_CP437_MONO8            CHARSET_CP437     + FFACE_MONO8        /* =0x02 */
#define FONT_NUPETSCII_OLIVETTITHIN CHARSET_NUPETSCII + FFACE_OLIVETTITHIN /* =0x11 */
#define FONT_CP437_OLIVETTITHIN     CHARSET_CP437     + FFACE_OLIVETTITHIN /* =0x12 */

#define BEL         0x07 //BEL	7	007	0x07	\a	^G	Terminal bell
#define BSP         0x08 //BS	8	010	0x08	\b	^H	Backspace
#define HT          0x09 //HT	9	011	0x09	\t	^I	Horizontal TAB
#define SPC         0x20

#define LF          0x0a //LF	10	012	0x0A	\n	^J	Linefeed (newline)
#define VT          0x0b //VT	11	013	0x0B	\v	^K	Vertical TAB
#define FF          0x0c //FF	12	014	0x0C	\f	^L	Formfeed (also: New page NP)
#define CR          0x0d //CR	13	015	0x0D	\r	^M	Carriage return

#define ESC         0x1b //ESC	27	033	0x1B	<none>	^[	Escape character
#define DEL         0x7f //DEL	127	177	0x7F	<none>	<none>	Delete character


typedef struct point {
  int x;
  int y;
} point_t;

const char* get_charset_name( uint8_t charset );
const char* get_font_face_name( uint8_t font_face );
const char* get_font_name( uint8_t font_id );
const bool has_charset( uint8_t font_id, uint8_t charset );


int replace_char(char *str, char orig, char rep);


#endif // _PICOTERM_STDDEF_H_
