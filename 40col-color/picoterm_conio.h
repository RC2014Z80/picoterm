#ifndef _PICOTERM_CONIO_H
#define _PICOTERM_CONIO_H

#include <stdint.h>
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_cursor.h"
#include "speccyfont.h"

/* Those are defined in the CMakeList !!!!
#define COLUMNS     40
#define ROWS        256 // yes this is 16 more than the number of scanlines we have
                        // for scrolling to work we need 8 spare pointers
#define VISIBLEROWS 29 // (as defined in the 80col version)
*/
#define TEXTROWS    29 // ROWS/8  // must be 2 text rows less than the max number of scanlines

uint32_t * wordsForRow(int y);

void conio_init( uint16_t fg_color, uint16_t bg_color );
void conio_reset();

char read_key();
void put_char(unsigned char ch,int x,int y);
void print_element (int x,int scanlineNumber, uint8_t* custom_bitmap );

void csr_blinking_task();

void clear_scanline_from_cursor(int r);
void clear_scanline_to_cursor(int r);
void clear_entire_scanline(int r);
void clear_scanline_between( int r, int y, int x, int to_x );
void copy_scanline_between( int r, int y, int to_x, int from_x, int x_len );
void clear_line_between( int y, int x, int to_x); // private usage
void copy_line_between( int y, int to_x, int from_x, int x_len ); // private


void clrscr(); // clear the primary screen
void clear_primary_screen();
// void clear_secondary_screen();
// void copy_main_to_secondary_screen();
// void copy_secondary_to_main_screen();
void clear_screen_from_cursor();
void clear_screen_to_cursor();
void shuffle_down(); // screen page scrolling UP
void shuffle_up();

void insert_line();
void delete_line();
void insert_lines(int n);
void delete_lines(int n);
void clear_line_from_cursor();
void clear_line_to_cursor();
void clear_entire_line();

void delete_chars(int n);
void erase_chars(int n);
void insert_chars(int n);

void clear_cursor();
void print_cursor();
void refresh_cursor();
void move_cursor_lf( bool reverse );
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
