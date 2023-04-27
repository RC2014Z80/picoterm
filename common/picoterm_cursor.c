/* ==========================================================================
           Manage the cursor Appearance/pisition for the PicoTerm
   ========================================================================== */

#include "picoterm_cursor.h"
#include "picoterm_stddef.h"
#include <stdio.h>
#include <stdbool.h>

bool is_blinking = false; // double check if it replicates the picoterm_cursor.c::get_cursor_blinking()

void cursor_state_init( cursor_state_t *this ){
  this->visible = true;
  this->blink_state = false;
  this->blinking_mode = true;
  //this->symbol = 143;
}

void cursor_term_init( cursor_term_t *this ){
  cursor_state_init( &(this->state) );
  this->symbol = 143;
  this->pos.x = 0;
  this->pos.y = 0;
}

char get_cursor_char( uint8_t font_id, uint8_t cursor_type ){
  // return the ASCII char for a given cursor.
  // Remark: Cursor Symbol = Cursor Character - 0x20
  char cursor_char;

  if( font_id==FONT_ASCII ){
          switch( cursor_type ){
            case CURSOR_TYPE_DEFAULT: // default configuration (underline blinking)
              cursor_char = 0x5F;
              break;
            case CURSOR_TYPE_BLOCK_BLINK:
            case CURSOR_TYPE_BLOCK_STEADY: // block
              cursor_char = 0x7F; //95;
              break;
            case CURSOR_TYPE_UNDERLINE_BLINK:
            case CURSOR_TYPE_UNDERLINE_STEADY:
              cursor_char = 0x5F;
              break;
            case CURSOR_TYPE_BAR_BLINK:
            case CURSOR_TYPE_BAR_STEADY:
              cursor_char = 0x5B;
              break;
            default:
              // return the CURSOR_TYPE_DEFAULT
              cursor_char = 0x5F;
          } // switch cursor type
  } else if ( has_charset( font_id, CHARSET_NUPETSCII) ){

            switch( cursor_type ){
              case CURSOR_TYPE_DEFAULT: // default configuration (underline blinking)
                cursor_char = 0xAF;
                break;
              case CURSOR_TYPE_BLOCK_BLINK:
              case CURSOR_TYPE_BLOCK_STEADY: // block
                cursor_char = 0x99; //95;
                break;
              case CURSOR_TYPE_UNDERLINE_BLINK:
              case CURSOR_TYPE_UNDERLINE_STEADY:
                cursor_char = 0xAF;
                break;
              case CURSOR_TYPE_BAR_BLINK:
              case CURSOR_TYPE_BAR_STEADY:
                cursor_char = 0xB4;
                break;
              default:
                // return the CURSOR_TYPE_DEFAULT
                cursor_char = 0xAF;
            } // switch cursor type
  } else if ( has_charset( font_id, CHARSET_CP437 ) ) {

              switch( cursor_type ){
                case CURSOR_TYPE_DEFAULT: // default configuration (underline blinking)
                  cursor_char = 0x5F;
                  break;
                case CURSOR_TYPE_BLOCK_BLINK:
                case CURSOR_TYPE_BLOCK_STEADY: // block
                  cursor_char = 0xDB; //95;
                  break;
                case CURSOR_TYPE_UNDERLINE_BLINK:
                case CURSOR_TYPE_UNDERLINE_STEADY:
                  cursor_char = 0x5F;
                  break;
                case CURSOR_TYPE_BAR_BLINK:
                case CURSOR_TYPE_BAR_STEADY:
                  cursor_char = 0xB3;
                  break;
                default:
                  // return the CURSOR_TYPE_DEFAULT
                  cursor_char = 0x5F;
              } // switch cursor type
  } else {
          // UNSUPPORTED Charset
          // The # is used to identify this use-case. Do not modify it, just made
          // a proper implementation in the switch case.
          cursor_char = 0x23;
  } // has_charset( font_id, ... )
  return cursor_char;
}


bool get_cursor_blinking( uint8_t nupetscii, uint8_t cursor_type ){
  // indicates if the cursor is blinking or not.
  return (cursor_type==CURSOR_TYPE_DEFAULT)||(cursor_type==CURSOR_TYPE_BLOCK_BLINK)||(cursor_type==CURSOR_TYPE_UNDERLINE_BLINK)||(cursor_type==CURSOR_TYPE_BAR_BLINK);
}
