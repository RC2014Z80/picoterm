#ifndef _PICOTERM_CURSOR_H_
#define _PICOTERM_CURSOR_H_

#include <stdint.h>
#include <stdbool.h>
/* ==========================================================================
            Manage the cursor Appearance/Position for the PicoTerm
   ========================================================================== */

// constants fits the ESC sequence value
//ESC [ 0 SP q  User Shape  Default cursor shape configured by the user
//ESC [ 1 SP q  Blinking Block  Blinking block cursor shape
//ESC [ 2 SP q  Steady Block  Steady block cursor shape
//ESC [ 3 SP q  Blinking Underline  Blinking underline cursor shape
//ESC [ 4 SP q  Steady Underline  Steady underline cursor shape
//ESC [ 5 SP q  Blinking Bar  Blinking bar cursor shape
//ESC [ 6 SP q  Steady Bar  Steady bar cursor shape
#define CURSOR_TYPE_DEFAULT 0
#define CURSOR_TYPE_BLOCK_BLINK 1
#define CURSOR_TYPE_BLOCK_STEADY 2
#define CURSOR_TYPE_UNDERLINE_BLINK 3
#define CURSOR_TYPE_UNDERLINE_STEADY 4
#define CURSOR_TYPE_BAR_BLINK 5
#define CURSOR_TYPE_BAR_STEADY 6

char get_cursor_char( uint8_t font_id, uint8_t cursor_type ); // return the ASCII char for a given type cursor
bool get_cursor_blinking( uint8_t font_id, uint8_t cursor_type ); // return true/false for a given type cursor

void make_cursor_visible(bool v);

bool get_csr_blink_state();
void set_csr_blink_state(bool state);

#endif
