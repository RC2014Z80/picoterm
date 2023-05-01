/* ==========================================================================
    ConIO configuration accessible to the application & private picoterm_conio.c
   ========================================================================== */

#ifndef _PICOTERM_CONIO_CONFIG_H
#define _PICOTERM_CONIO_CONFIG_H

#include "picoterm_cursor.h"

/* console IO configuration */
typedef struct picoterm_conio_config {
  bool rvs; // draw in reverse
  bool blk; // draw in blinking
  bool just_wrapped;
  bool wrap_text;   // terminal configured to warp_text around
  uint8_t dec_mode; // current DEC mode (ligne drawing single/double/none)
  uint8_t ansi_font_id; // ID of the ANSI Graphical font to use
  cursor_term_t cursor; // full definition of a terminal cursor
} picoterm_conio_config_t;


void conio_config_init();

#endif
