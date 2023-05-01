/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading key from keyboard, etc
   ========================================================================== */

#include <stdbool.h>
#include <string.h>
#include "../common/picoterm_conio_config.h"
#include "picoterm_conio.h"
#include "picoterm_core.h" // scanline functions
#include "../common/keybd.h"
#include "../common/picoterm_dec.h" // DEC codification
#include "stdlib.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "bsp/board.h" // board_millis()

#include "../common/picoterm_debug.h"

/* picoterm_cursor.c */
extern bool is_blinking;

/* picoterm_conio_config.c */
extern picoterm_conio_config_t conio_config;

// Current color
uint16_t foreground_colour;
uint16_t background_colour;

// saved cursor
struct point __saved_csr = {0,0};

typedef struct scanline { uint16_t pixels[(COLUMNS*8)]; } scanline;
static struct scanline *ptr[ROWS];

uint16_t  __chr_under_csr[64] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                0,0,0,0};
bool __has_chr_under_csr = false; // indicates is __chr_under_csr is supposed to contains a char (so not all black or bg_color)


void conio_init( uint16_t fg_color, uint16_t bg_color ){
  foreground_colour = fg_color;
  background_colour = bg_color;
  for(int i=0;i<64;i++)
    __chr_under_csr[i] = bg_color;

  for(int c=0;c<ROWS;c++){
      struct scanline *newRow;
      /* Create structure in memory */
      newRow=(struct scanline *)malloc(sizeof(struct scanline));
      if(newRow==NULL)
      {
          exit(1);
      }
      ptr[c] = newRow;
  }
}

void conio_reset(){
  // Reset the terminal
	conio_config_init();
  /* conio_config.rvs = false;
  conio_config.blk = false;
  conio_config.wrap_text = true;
  conio_config.just_wrapped = false;
  conio_config.dec_mode = DEC_MODE_NONE; // single/double lines
	*/
  // initialized @ init()
  // conio_config.ansi_font_id = FONT_NUPETSCII; // selected font_id for graphical operation

  /*conio_config.cursor.state.visible = true;
  conio_config.cursor.state.blink_state = false; // blinking cursor is in hidden state
  conio_config.cursor.state.blinking_mode = true;
	*/
  conio_config.cursor.symbol = 143;

  clrscr();
  //clear_secondary_screen();
}

char read_key(){
  // read a key from input buffer (the keyboard or serial line). This is used
  // for menu handling. Return 0 if no char available
  if( key_ready()==false )
    return 0;
  return read_key_from_buffer();
}

void print_element (int x,int scanlineNumber, uint8_t* custom_bitmap ){
  // Used to print a bitmap of custom bitmap as declared in picoterm_core.h
  uint8_t rawdata;
  for (int r=0;r<6;r++){
      rawdata = custom_bitmap[r];  // at startup, first char in custom bitmaps is the block
      for(int bit=0;bit<6;bit++){
          if(rawdata & ( 0b10000000 >> bit)){
              ptr[scanlineNumber+r]->pixels[x+bit] = foreground_colour;
          }
          else{
              // ptr[scanlineNumber]->pixels[x+bit] = palette[0];
          }
      }
  }

}

// most important accessor
uint32_t * wordsForRow(int y){
    return (uint32_t * )&ptr[y]->pixels[0];
}

void put_char(unsigned char ch,int x,int y){
    // basically put character at chr position x,y

    // ch is 'screen code, or asc-32
    // so starts at 0, our space.
    // therefore, custom chars start at 96

    // ignore requests where character is out of range
    if(x>=COLUMNS || y>=TEXTROWS || x<0 || y<0) return;

    // each row is a scanline, made of uin16_t pixels

        uint8_t rawdata;
        int scanlineNumber;
        int characterPosition;
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (y*8)+r;
            characterPosition = (x*8);

            if(ch<96){
                rawdata = speccy_bitmap[(ch*8)+r];
            }
            else{
                rawdata = custom_bitmap[((ch-96)*8)+r];
            }

            for(int bit=0;bit<8;bit++){
                if(characterPosition+bit<(COLUMNS*8)){
                    //if(((rawdata & (0b10000000 >> bit)!=0) && !rvs)
                    //    || ((rawdata & (0b10000000 >> bit)==0) && rvs)
                    //)
                    if(rawdata & (0b10000000 >> bit))
                    {
                        if(!conio_config.rvs){
                             ptr[scanlineNumber]->pixels[characterPosition+bit]
                                = foreground_colour;
                        }
                        else{
                            ptr[scanlineNumber]->pixels[characterPosition+bit]
                            = background_colour;
                        }

                    }
                    else{
                        if(conio_config.rvs){
                            ptr[scanlineNumber]->pixels[characterPosition+bit]
                                = foreground_colour;
                        }
                        else{
                            ptr[scanlineNumber]->pixels[characterPosition+bit]
                            = background_colour;
                        }
                    }
                    if (conio_config.just_wrapped)
                        conio_config.just_wrapped = false;
                }
            }
        }
}


void clear_scanline_from_cursor(int r){
    // TODO: rely on clear_scanline_between
    uint16_t *sl = &ptr[r]->pixels[conio_config.cursor.pos.x*8];
    for(int i=conio_config.cursor.pos.x*8;i<COLUMNS*8;i++){
        *sl++ = background_colour;
    }
}

void clear_scanline_to_cursor(int r){
    // TODO: rely on clear_scanline_between
    uint16_t *sl = &ptr[r]->pixels[0];
    for(int i=0;i<conio_config.cursor.pos.x*8;i++){
        *sl++ = background_colour;
    }
}

void clear_scanline_between( int r, int y, int x, int to_x ){
  // mimic the r' clear_scanline_from_cursor BUT gets y,x position instead of
  // cursor location AND can clear few column (instead of until-end-of-line)
  uint16_t *sl = &ptr[r]->pixels[x*8];
  for(int i=x*8;i<to_x*8;i++){
      *sl++ = background_colour;
  }
}

void copy_scanline_between( int r, int y, int to_x, int from_x, int x_len ){
  // just copy the content of the r' scanline from_x position into the to_x position
  // Just copy x_len characters from_x into to_x
  if( to_x < from_x ) {
    // Forward copy
    uint16_t *sl_to = &ptr[r]->pixels[to_x*8];
    uint16_t *sl_from = &ptr[r]->pixels[from_x*8];
    for(int i=to_x*8;i<(to_x+x_len)*8;i++)
        *sl_to++ = *sl_from++;
  }
  else {
    // Backward copy
    uint16_t *sl_to = &ptr[r]->pixels[(to_x+x_len)*8];
    uint16_t *sl_from = &ptr[r]->pixels[(from_x+x_len)*8];
    for(int i=(to_x+x_len)*8;i>to_x*8;i--)
        *sl_to-- = *sl_from--;
  }
}


void clear_entire_scanline(int r){
    // can't use the fast memset method here because we want to fill with 16 bit values
    uint16_t *sl = &ptr[r]->pixels[0];
    for(int i=0;i<COLUMNS*8;i++){
        *sl++ = background_colour;
    }
}


// === Screen based function ===================================================
void clrscr(){ // standard definition for clear screen
  /* From 40col */
  for(int r=0;r<ROWS;r++){
      clear_entire_scanline(r);
  }
}

void clear_primary_screen(){
  clrscr();
}

void clear_screen_from_cursor(){
    clear_line_from_cursor();
    for(int r=((conio_config.cursor.pos.y+1)*8);r<TEXTROWS*8;r++){
        clear_entire_scanline(r);
    }
}

void clear_screen_to_cursor(){
    clear_line_to_cursor();
    for(int r=((conio_config.cursor.pos.y+1)*8);r<TEXTROWS*8;r++){
        clear_entire_scanline(r);
    }
}

void shuffle_down(){
    // this is our scroll DOWN for the content
    // because we're using pointers to rows, we only need to shuffle the array of pointers
    // with textmode it's just ~30 pointers. Colourmode it's every scanline (~240)
    // recycle first line, and clear it.

    for(int r=0;r<8;r++){
        ptr[(ROWS-8)+r]=ptr[r];
        clear_entire_scanline(r);
    }

    for(int r=0;r<ROWS-8;r++){
        ptr[r]=ptr[r+8];
    }

    // finally recycled lines need blanking
    for(int r=0;r<8;r++){
        clear_entire_scanline((ROWS-8)+r);
    }
}

void shuffle_up(){
    // this is our scroll UP
    // (domeu: adapted from shuffle_down)
    debug_print( "picoterm_conio: shuffle_up() - scroll_down3 test have issue");
    for(int r=0;r<8;r++){
        ptr[r]=ptr[(ROWS-8)+r];
        clear_entire_scanline((ROWS-8)+r);
    }

    for(int r=ROWS;r>8;r--){
        ptr[r]=ptr[r-8];
    }

    // finally recycled lines need blanking
    for(int r=0;r<8;r++){
        clear_entire_scanline(r);
    }
}

// === Line based function =====================================================


void insert_line(){
    struct scanline *ptrTemp[8];
    // store the last LINE pointer for further recyle
    int i = 0;
    for( int r=ROWS-8; r<ROWS; r++ )
        ptrTemp[i++] = ptr[r];
    // move scan lines down of ONE LiNE line
    for( int r=ROWS; r>(conio_config.cursor.pos.y*8)+8; r-- )
        ptr[r] = ptr[r-8];
    // finally recycled temporary stored scanline
    for( int i = 0; i < 8; i++ ){
        ptr[(conio_config.cursor.pos.y*8)+i]=ptrTemp[i];
        clear_entire_scanline( (conio_config.cursor.pos.y*8)+i );
    }
}

void delete_line(){
    struct scanline *ptrTemp[8];
    // store the "to delete" LINE pointer for further recyle
    int i = 0;
    for( int r=conio_config.cursor.pos.y*8; r<(conio_config.cursor.pos.y*8)+8; r++ )
        ptrTemp[i++] = ptr[r];
    // move scan lines up of ONE LiNE line
    for( int r=(conio_config.cursor.pos.y*8); r<ROWS-8; r++ )
        ptr[r] = ptr[r+8];
    // finally recycled temporary stored scanline
    i = 0;
    for( int r = ROWS-8; r < ROWS; r++ ){
        ptr[r]=ptrTemp[i++];
        clear_entire_scanline( r );
    }
}

void insert_lines(int n){
    for (int i = 0; i < n; i++)
    {
        insert_line();
    }
}

void delete_lines(int n){
    for (int i = 0; i < n; i++)
    {
        delete_line();
    }
}

void clear_line_from_cursor(){
    for(int r=(conio_config.cursor.pos.y*8);r<(conio_config.cursor.pos.y*8)+8;r++){
        clear_scanline_from_cursor(r);
    }
}

void clear_line_between( int y, int x, int to_x){
  // mimic clear_line_from_cursor but got y, x as starting position THEN reduce
  // the clearing between x and to_x column

  // keeps value into a good range
  if( to_x > COLUMNS)
    to_x = COLUMNS;

  for(int r=(y*8);r<(y*8)+8;r++) // iterate the 8 scanlines for the line
      clear_scanline_between(r, y, x, to_x );
}

void copy_line_between( int y, int to_x, int from_x, int x_len ){
  // Copy part of ONE line content between two X distinct position.
  // -> copy x_len chars of line Y from from_x position into to_x position

  // keeps value into correct range
  if( (to_x + x_len) > COLUMNS )
    x_len = COLUMNS - to_x;
  if( (from_x + x_len) > COLUMNS )
    x_len = COLUMNS - from_x;

  for(int r=(y*8);r<(y*8)+8;r++) // iterate the 8 scanlines for the line
    copy_scanline_between( r, y, to_x, from_x, x_len );
}

void clear_line_to_cursor(){
    for(int r=(conio_config.cursor.pos.y*8);r<(conio_config.cursor.pos.y*8)+8;r++){
      clear_scanline_to_cursor(r);
    }
}

void clear_entire_line(){
    for(int r=(conio_config.cursor.pos.y*8);r<(conio_config.cursor.pos.y*8)+8;r++){
        clear_entire_scanline(r);
    }
}

// === Char based function =====================================================

void delete_chars(int n){
  // Delete character under the cursor and move the remaining on the line to
  // the left.
  // copy the "not deleted" char in the current cursor position
  copy_line_between( conio_config.cursor.pos.y, conio_config.cursor.pos.x, conio_config.cursor.pos.x+n, n );
  // Clear the end of the line
  clear_line_between( conio_config.cursor.pos.y, conio_config.cursor.pos.x+n, COLUMNS );
}

void erase_chars(int n) {
  // Erase/Clean the n chars on the right of the cursor (cursor included)
   clear_line_between( conio_config.cursor.pos.y, conio_config.cursor.pos.x, conio_config.cursor.pos.x+n);
}

void insert_chars(int n) {
  copy_line_between( conio_config.cursor.pos.y, conio_config.cursor.pos.x+n, conio_config.cursor.pos.x, COLUMNS-conio_config.cursor.pos.x-n  );
  clear_line_between( conio_config.cursor.pos.y, conio_config.cursor.pos.x, conio_config.cursor.pos.x+n);
}

// === Cursor based function ===================================================

void clear_cursor(){
  // Just restore memoriser character (buffer content) right in place on the screen
  int scanlineNumber;
  int characterPosition;
  int cursor_buffer_counter;
  uint16_t pixel;

  cursor_buffer_counter = 0;
  for(int r=0;r<8;r++){   // r is a row offset
      scanlineNumber = (conio_config.cursor.pos.y*8)+r;
      characterPosition = (conio_config.cursor.pos.x*8);
      for(int bit=0;bit<8;bit++){
          pixel = __chr_under_csr[cursor_buffer_counter++];
          ptr[scanlineNumber]->pixels[characterPosition+bit] = pixel;
      }
  }
  // Reset the flag for print_cursor
  __has_chr_under_csr = false;
}


void print_cursor(){
  int scanlineNumber;
  int characterPosition;
  int cursor_buffer_counter;
  uint16_t pixel, newPixel;
  int pixel_count;

  // Make copy of the character under the cursor
  // (don't make it twice, be sure we called clear_cursor before another copy)
  if (__has_chr_under_csr==false ){
        cursor_buffer_counter = 0;
        pixel_count = 0;
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (conio_config.cursor.pos.y*8)+r;
            characterPosition = (conio_config.cursor.pos.x*8);
            for(int bit=0;bit<8;bit++){
                  pixel = ptr[scanlineNumber]->pixels[characterPosition+bit];
                  __chr_under_csr[cursor_buffer_counter++]=pixel;
                  // count number of colored pixels
                  if( (pixel!=0) && (pixel!=background_colour) ){
                     pixel_count+=1;
                  }
            }
            // Whem mode than 10 colored pixel => We have a char under the cursor
            __has_chr_under_csr = (pixel_count > 10);
        }
  }

  if(conio_config.cursor.state.visible==false || (conio_config.cursor.state.blinking_mode && conio_config.cursor.state.blink_state)) return;

  if( !(__has_chr_under_csr) ){
        // display cursor Symbol
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (conio_config.cursor.pos.y*8)+r;
            characterPosition = (conio_config.cursor.pos.x*8);
            for(int bit=0;bit<8;bit++)
                  ptr[scanlineNumber]->pixels[characterPosition+bit] = foreground_colour ; // Bloc cursor
        }
  } else {
        // Display the invert of memorised char
        // put back what's in the cursor_buffer
        cursor_buffer_counter = 0;
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (conio_config.cursor.pos.y*8)+r;
            characterPosition = (conio_config.cursor.pos.x*8);
            for(int bit=0;bit<8;bit++){
                pixel = __chr_under_csr[cursor_buffer_counter++];
                newPixel = (pixel==background_colour)|(pixel==0) ? foreground_colour : background_colour;
                ptr[scanlineNumber]->pixels[characterPosition+bit] = newPixel;

            }
        }
  }

}

void refresh_cursor(){
  clear_cursor();
  print_cursor();
}

void cursor_reverse_lf(){
  if(conio_config.cursor.pos.y > 0)
      move_cursor_at(conio_config.cursor.pos.y, conio_config.cursor.pos.x + 1);
  else
      shuffle_up();
}

void move_cursor_lf( bool reverse ){
  if( reverse ){ // move cursor up
    cursor_reverse_lf();
    return;
  }

  // move cursor down
  if(conio_config.wrap_text){
    if(!conio_config.just_wrapped){
      if(conio_config.cursor.pos.y==VISIBLEROWS-1){ // visiblerows is the count, csr is zero based
        shuffle_down();
       }
       else {
        conio_config.cursor.pos.y++;
       }
    }
    else
      conio_config.just_wrapped = false;
  }
  else{
     if(conio_config.cursor.pos.y==VISIBLEROWS-1){ // visiblerows is the count, csr is zero based
        shuffle_down();
     }
     else {
        conio_config.cursor.pos.y++;
     }
  }
}

void wrap_constrain_cursor_values(){
  if(conio_config.cursor.pos.x>=COLUMNS) {
    conio_config.cursor.pos.x=0;
    if(conio_config.cursor.pos.y==VISIBLEROWS-1){   // visiblerows is the count, csr is zero based
      shuffle_down();
    }
    else{
      conio_config.cursor.pos.y++;
    }
    conio_config.just_wrapped = true;
  }
}

void constrain_cursor_values(){
    if(conio_config.cursor.pos.x<0) conio_config.cursor.pos.x=0;
    if(conio_config.cursor.pos.x>=COLUMNS) conio_config.cursor.pos.x=COLUMNS-1;
    if(conio_config.cursor.pos.y<0) conio_config.cursor.pos.y=0;
    if(conio_config.cursor.pos.y>=VISIBLEROWS) conio_config.cursor.pos.y=VISIBLEROWS-1;
}

void move_cursor_at(int y, int x){
  // Set the cursor at position Y:1..nRows, X:1..nRows
  y--;
  x--;

  // Moves the cursor to row n, column m
  // The values are 1-based, and default to 1

  // these are zero based
  conio_config.cursor.pos.x = x;
  conio_config.cursor.pos.y = y;
  constrain_cursor_values();
}

void move_cursor_home(){
    move_cursor_at(1, 1);
}

void move_cursor_up(int n){
    if(n==0)n=1;
    conio_config.cursor.pos.y -= n;
    constrain_cursor_values();
}

void move_cursor_down(int n){
    if(n==0)n=1;
    conio_config.cursor.pos.y += n;
    constrain_cursor_values();  // todo: should possibly do a scroll up?
}

void move_cursor_forward(int n){
    if(n==0)n=1;
    conio_config.cursor.pos.x += n;
    constrain_cursor_values();
}

void move_cursor_backward(int n){
    if(n==0)n=1;
    conio_config.cursor.pos.x -= n;
    constrain_cursor_values();
}

void cursor_visible(bool v){
    conio_config.cursor.state.visible=v;
		if( v==true )
			print_cursor();
	  else
			clear_cursor();
}

bool cursor_blink_state() {
  return conio_config.cursor.state.blink_state;
}
void set_cursor_blink_state( bool state ) {
  // is the Blinking cursor should currently be visible or not visible
  conio_config.cursor.state.blink_state = state;
}

void save_cursor_position(){
  __saved_csr.x = conio_config.cursor.pos.x;
  __saved_csr.y = conio_config.cursor.pos.y;
}

void restore_cursor_position(){
  conio_config.cursor.pos.x = __saved_csr.x;
  conio_config.cursor.pos.y = __saved_csr.y;
}

void reset_saved_cursor(){
  __saved_csr.x = conio_config.cursor.pos.x;
  __saved_csr.y = conio_config.cursor.pos.y;
}

//--------------------------------------------------------------------+
//  ConIO Specific Tasks
//--------------------------------------------------------------------+

void csr_blinking_task() {
  const uint32_t interval_ms_csr = 525;
  static uint32_t start_ms_csr = 0;

  // Blink every interval ms
  if ( board_millis() - start_ms_csr > interval_ms_csr) {

    start_ms_csr += interval_ms_csr;

    is_blinking = !is_blinking;
    set_cursor_blink_state( 1 - cursor_blink_state() );

    refresh_cursor();
  }
}
