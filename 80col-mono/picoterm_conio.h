/* ==========================================================================
    Manage the basic console interaction for the PicoTerm software
          * put_char on the screen
					* read_key from keyboard
          * managing everything regarding the Console display (moving
					  cursor, inverting content, etc)

		Advanced interaction with the console (printing string, request input
	  is provided by common/picoterm_stdlio.c)
   ========================================================================== */

#ifndef _PICOTERM_CONIO_H
#define _PICOTERM_CONIO_H

#include <stdint.h>
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_cursor.h"
#include "../common/picoterm_conio_config.h"

/* Those are defined in the CMakeList !!!!
#define COLUMNS     80
#define ROWS        34
#define VISIBLEROWS 30
*/



typedef struct row_of_text {
  unsigned char slot[COLUMNS];
  unsigned char inv[COLUMNS];
  unsigned char blk[COLUMNS];
} row_of_text_t;

// array of pointers, each pointer points to a row structure
typedef row_of_text_t *array_of_row_text_pointer[ROWS];

void conio_init( uint8_t ansi_font_id ); // allocate required ressources
void conio_reset( char default_cursor_symbol );


// Read a key from input buffer (the keyboard or serial line). Return 0 if no
// char available (this is used by the menu handling)
char read_key();
void put_char(unsigned char ch,int x,int y);
//void print_string(char str[]); --> picoterm_stdio.C
void print_nupet(char str[], uint8_t font_id ); // Print a NupetSCII encoded string to terminal with current font_id

void csr_blinking_task();

void clrscr(); // clear the primary screen
void clear_primary_screen();
void clear_secondary_screen();
void copy_main_to_secondary_screen();
void copy_secondary_to_main_screen();
void clear_screen_from_cursor();
void clear_screen_to_cursor();
void shuffle_down(); // screen scrolling
void shuffle_up();

void insert_line();
void delete_line();
void insert_lines(int n);
void delete_lines(int n);
void clear_line_from_cursor();
void clear_line_to_cursor();
void clear_entire_line();

void set_char(int x, int y, char ch );
void set_reverse( int x, int y, bool state ); // indicate when a position must be printed as Reverse
void set_blinking( int x, int y, bool state ); // indicate when a position must be printed as Blinking
unsigned char slop_character(int x,int y);
unsigned char * slotsForRow(int y);
unsigned char * slotsForInvRow(int y);
unsigned char * slotsForBlkRow(int y);
void delete_chars(int n);
void erase_chars(int n);
void insert_chars(int n);
unsigned char inv_character(int x,int y);
unsigned char blk_character(int x,int y);

void clear_cursor();
void print_cursor();
void refresh_cursor();
void move_cursor_lf( bool reverse ); // move cursor
void move_cursor_at(int y, int x);
void move_cursor_home();
void move_cursor_up(int n);
void move_cursor_down(int n);
void move_cursor_forward(int n);
void move_cursor_backward(int n);
void constrain_cursor_values();
void wrap_constrain_cursor_values();

void cursor_visible(bool v);
bool cursor_blink_state(); // is the blinking cursor currently visible or hidden ?
void set_cursor_blink_state(bool state);
void save_cursor_position();
void restore_cursor_position();
void reset_saved_cursor();

#endif
