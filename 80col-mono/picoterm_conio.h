/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading from keyboard, etc
   ========================================================================== */


#ifndef _PICOTERM_CONIO_H
#define _PICOTERM_CONIO_H

#include <stdint.h>
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_cursor.h"

#define COLUMNS     80
#define ROWS        34
#define VISIBLEROWS 30

/* console IO configuration */
typedef struct picoterm_conio_config {
  bool rvs; // draw in reverse
  bool blk; // draw in blinking
  bool just_wrapped;
  bool wrap_text;   // terminal configured to warp_text around
	uint8_t dec_mode; // current DEC mode (ligne drawing single/double/none)
	uint8_t ansi_font_id; // ID of the ANSI Graphical font to use
	point_t cursor; // terminal cursor
	cursor_state_t cursor_state; // blinking, visible, etc
} picoterm_conio_config_t;

typedef struct row_of_text {
  unsigned char slot[COLUMNS];
  unsigned char inv[COLUMNS];
  unsigned char blk[COLUMNS];
} row_of_text_t;

// array of pointers, each pointer points to a row structure
typedef row_of_text_t *array_of_row_text_pointer[ROWS];

void conio_init( uint8_t ansi_font_id ); // allocate required ressources
void conio_reset( char default_cursor_symbol );

void print_string(char str[]);
void __print_string(char str[], bool strip_graphical );

// Read a key from input buffer (the keyboard or serial line). Return 0 if no
// char available (this is used by the menu handling)
char read_key();

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
void slip_character(unsigned char ch,int x,int y);

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

void make_cursor_visible(bool v);
bool get_csr_blink_state();
void set_csr_blink_state(bool state);

#endif
