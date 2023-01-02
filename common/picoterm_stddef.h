#ifndef _PICOTERM_STDDEF_H_
#define _PICOTERM_STDDEF_H_

#include <stdint.h>

/* ==========================================================================
                   Standard definition for Picoterm
   ========================================================================== */

#define FONT_ASCII     0 /* 7 bits fonts (8th bit do reverse the char) */
#define FONT_NUPETSCII 1 /* ANSI is 8 bits */
#define FONT_CP437     2

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

const char* get_font_name( uint8_t font_id );

int replace_char(char *str, char orig, char rep);


#endif // _PICOTERM_STDDEF_H_
