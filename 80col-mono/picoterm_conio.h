/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading from keyboard, etc
   ========================================================================== */


#ifndef _PICOTERM_CONIO_H
#define _PICOTERM_CONIO_H


void print_string(char str[]);
void __print_string(char str[], bool strip_graphical );

// Read a key from input buffer (the keyboard or serial line). Return 0 if no
// char available (this is used by the menu handling)
char read_key();


#endif
