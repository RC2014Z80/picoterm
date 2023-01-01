/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading key from keyboard, etc
   ========================================================================== */

#include <stdbool.h>
#include <string.h>
#include "picoterm_conio.h"
#include "../common/picoterm_stddef.h"
#include "../common/keybd.h" // Keyboard device
#include "picoterm.h"


/* picoterm_cursor.c */
extern bool cursor_blinking;

void __print_string(char str[], bool strip_graphical ){
   // remove the graphical/NuPetScii extended charset from a string and replace them with
  // more convenient.
  // This function is used by the configuration screen. See display_config().
  char c;
  for(int i=0;i<strlen(str);i++){
      c = str[i];
      if( strip_graphical )
        switch (c) {
          case '\x0A6':
              c = ' ';
              break;
          case '\x0C2':
          case '\x0E0':
              c = '|';
              break;
          case '\x0C3':
          case '\x0E1':
              c = '-';
              break;
          case '\x083':
              c = '*'; // replace a bullet
              break;
          case '\x0B0':
          case '\x0AD':
          case '\x0BD':
          case '\x0AE':
          case '\x0AB':
          case '\x0B3':
          case '\x0E8':
          case '\x0E9':
          case '\x0B2':
          case '\x08A':
          case '\x0E7':
          case '\x0E5':
          case '\x0E2':
          case '\x0E4':
          case '\x0DB':
          case '\x0B1':
              c = '+';
              break;
          case '\x0D1':
              c = '>'; // Replace a "selected item" marker
              break;
          default:
              break;
        }
      handle_new_character( c );
  }
}

void print_string(char str[] ){
  __print_string( str, FONT_ASCII );
}

char read_key(){
  // read a key from input buffer (the keyboard or serial line). This is used
  // for menu handling. Return 0 if no char available
  if( key_ready()==false ) return 0;

  if(cursor_blinking) cursor_blinking = false;

  return read_key_from_buffer();
}
