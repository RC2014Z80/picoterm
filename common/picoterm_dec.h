#ifndef _PICOTERM_DEC_H_
#define _PICOTERM_DEC_H_

#include <stdint.h>
/* ==========================================================================
                 Manage the DEC line drawing for the PicoTerm
   ========================================================================== */

#define DEC_MODE_NONE         0
#define DEC_MODE_SINGLE_LINE  1
#define DEC_MODE_DOUBLE_LINE  2

char get_dec_char( uint8_t font_id, uint8_t dec_mode, char _char ); // return the ASCII char for a given type of line drawing

#endif
