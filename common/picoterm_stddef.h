#ifndef _PICOTERM_STDDEF_H_
#define _PICOTERM_STDDEF_H_

/* ==========================================================================
                   Standard definition for Picoterm
   ========================================================================== */

#define FONT_ANSI      0 /* 7 bits fonts (8th bit do reverse the char) */
#define FONT_NUPETSCII 1
#define FONT_CP437     2

typedef struct point {
  int x;
  int y;
} point_t;

#endif // _PICOTERM_STDDEF_H_
