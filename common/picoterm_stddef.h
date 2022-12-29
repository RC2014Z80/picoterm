#ifndef _PICOTERM_STDDEF_H_
#define _PICOTERM_STDDEF_H_

#include <stdint.h>

/* ==========================================================================
                   Standard definition for Picoterm
   ========================================================================== */

#define FONT_ASCII     0 /* 7 bits fonts (8th bit do reverse the char) */
#define FONT_NUPETSCII 1 /* ANSI is 8 bits */
#define FONT_CP437     2

typedef struct point {
  int x;
  int y;
} point_t;

const char* get_font_name( uint8_t font_id );

#endif // _PICOTERM_STDDEF_H_
