/* ==========================================================================
           Manage the cursor Appearance/pisition for the PicoTerm
   ========================================================================== */

#include "picoterm_cursor.h"
#include "picoterm_stddef.h"
#include <stdio.h>

bool cursor_visible;              // should the cursor be visible on the terminal
bool cursor_blinking = false;     // Is the cursor do blink now ? (see csr_)
bool cursor_blinking_mode = true; // do we want the cursor to be BLINKING or STEADY
char cursor_symbol = 143;         // index in charset for the cursor

struct point csr = {0,0}; // Cursor position
struct point saved_csr = {0,0};

char get_cursor_char( uint8_t nupetscii, uint8_t cursor_type ){
  // return the ASCII char for a given cursor.
  // Remark: Cursor Symbol = Cursor Character - 0x20
  char cursor_char;
  switch( cursor_type ){
    case CURSOR_TYPE_DEFAULT: // default configuration (underline blinking)
      cursor_char = nupetscii==1 ? 0xAF : 0x5F;
      break;
    case CURSOR_TYPE_BLOCK_BLINK:
    case CURSOR_TYPE_BLOCK_STEADY: // block
      cursor_char = nupetscii==1 ? 0x99 : 0x7F; //95;
      break;
    case CURSOR_TYPE_UNDERLINE_BLINK:
    case CURSOR_TYPE_UNDERLINE_STEADY:
      cursor_char = nupetscii==1 ? 0xAF : 0x5F;
      break;
    case CURSOR_TYPE_BAR_BLINK:
    case CURSOR_TYPE_BAR_STEADY:
      cursor_char = nupetscii==1 ? 0xB4 : 0x5B;
      break;
    default:
      // return the CURSOR_TYPE_DEFAULT
      cursor_char = nupetscii==1 ? 0xAF : 0x5F;
  }
  return cursor_char;
}


bool get_cursor_blinking( uint8_t nupetscii, uint8_t cursor_type ){
  // indicates if the cursor is blinking or not.
  return (cursor_type==CURSOR_TYPE_DEFAULT)||(cursor_type==CURSOR_TYPE_BLOCK_BLINK)||(cursor_type==CURSOR_TYPE_UNDERLINE_BLINK)||(cursor_type==CURSOR_TYPE_BAR_BLINK);
}

void make_cursor_visible(bool v){
    cursor_visible=v;
}

bool get_csr_blink_state() {
  return cursor_blinking; 
}
void set_csr_blink_state(bool state) {
  cursor_blinking = state;
}
