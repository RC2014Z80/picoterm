/* ==========================================================================
    Manage the basic console interaction for the PicoTerm software
          * put_char on the screen
					* read_key from keyboard
          * managing everything regarding the Console display (moving
					  cursor, inverting content, etc)

		Advanced interaction with the console (printing string, request input
	  is provided by common/picoterm_stdlio.c)
   ========================================================================== */

#include <stdbool.h>
#include <string.h>
#include "picoterm_conio.h"
#include "../common/picoterm_conio_config.h"
#include "../common/picoterm_cursor.h"
#include "../common/picoterm_dec.h"
#include "../common/picoterm_stddef.h"
#include "../common/keybd.h" // Keyboard device
#include "picoterm_core.h"
#include "../common/picoterm_config.h"
#include <stdlib.h>
#include "bsp/board.h" // board_millis()

/* picoterm_cursor.c */
extern bool is_blinking;

/* picoterm_config.c */
extern picoterm_config_t config; // required to read config.font_id

/* picoterm_conio_config.c */
extern picoterm_conio_config_t conio_config;

array_of_row_text_pointer ptr;           // primary screen content
array_of_row_text_pointer secondary_ptr; // secondary screen content

// Private members
unsigned char __chr_under_csr; // Character under the cursor
bool __inv_under_csr;
bool __blk_under_csr;

// saved cursor
struct point __saved_csr = {0,0};

void conio_init( uint8_t ansi_font_id ){
  // Initialize the ConIO ressources
  for(int c=0;c<ROWS;c++){
      struct row_of_text *newRow;
      /* Create structure in memory */
      newRow=(struct row_of_text *)malloc(sizeof(struct row_of_text));
      if(newRow==NULL) exit(1);
      ptr[c] = newRow;

      /* Create structure in memory */
      newRow=(struct row_of_text *)malloc(sizeof(struct row_of_text));
      if(newRow==NULL) exit(1);
      secondary_ptr[c] = newRow;
  }
  conio_config.ansi_font_id = ansi_font_id;
  cursor_term_init( &(conio_config.cursor) );
}

void conio_reset( char default_cursor_symbol ){
  // Reset the terminal
  /*conio_config.rvs = false;
  conio_config.blk = false;
  conio_config.wrap_text = true;
  conio_config.just_wrapped = false;
  conio_config.dec_mode = DEC_MODE_NONE; // single/double lines */
	conio_config_init();
  // initialized @ init()
  // conio_config.ansi_font_id = FONT_NUPETSCII; // selected font_id for graphical operation

  __chr_under_csr = 0;
  __inv_under_csr = 0;
  __blk_under_csr = 0;

  /* conio_config.cursor.state.visible = true;
  conio_config.cursor.state.blink_state = false; // blinking cursor is in hidden state
  conio_config.cursor.state.blinking_mode = true; */
  conio_config.cursor.symbol = default_cursor_symbol;

  clrscr();
  clear_secondary_screen();
}


//void __print_string(char str[], bool strip_graphical ){
void print_nupet(char str[], uint8_t font_id ){
  // Print a NupetSCII encoded string to terminal with current font_id selected.
  // translate the string characters to the target font_id  at the best.
  //
  // This function is used by the configuration screen. See display_config().
  char c;
  for(int i=0;i<strlen(str);i++){
      c = str[i];
			if( has_charset(font_id,CHARSET_CP437) ){
					switch (c) {
						case '\x0AA':
								c = '\x0B3';
								break;
						case '\x0AB':
								c = '\x0C3';
								break;
						case '\x0AD':
								c = '\x0C0';
								break;
						case '\x0AE':
								c = '\x0BF';
								break;
						case '\x0AF':
								c = '\x0C4';
								break;
						case '\x0B0':
								c = '\x0DA';
								break;
						case '\x0B1':
								c = '\x0C1';
								break;
						case '\x0B2':
								c = '\x0C2';
								break;
						case '\x0B3':
								c = '\x0B4';
								break;
						case '\x0B4':
								c = '\x0B3';
								break;
						case '\x0BA':
								c = '\x0D9';
								break;
						case '\x0BD':
								c = '\x0D9';
								break;
						case '\x0C2':
								c = '\x0B3';
								break;
						case '\x0C3':
								c = '\x0C4';
								break;
						case '\x0C9':
								c = '\x0BF';
								break;
						case '\x0CA':
								c = '\x0CA';
								break;
						case '\x0CB':
								c = '\x0D9';
								break;
						case '\x0CC':
								c = '\x0C0';
								break;
						case '\x0CF':
								c = '\x0DA';
								break;
						case '\x0D0':
								c = '\x0BF';
								break;
						case '\x0D5':
								c = '\x0DA';
								break;
						case '\x0D7':
								c = '\x0AF';
								break;
						case '\x0DB':
								c = '\x0C5';
								break;
						case '\x0E0':
								c = '\x0BA';
								break;
						case '\x0E1':
								c = '\x0CD';
								break;
						case '\x0E2':
								c = '\x0C9';
								break;
						case '\x0E3':
								c = '\x0CB';
								break;
						case '\x0E4':
								c = '\x0BB';
								break;
						case '\x0E5':
								c = '\x0C8';
								break;
						case '\x0E6':
								c = '\x0CA';
								break;
						case '\x0E7': // --
								c = '\x0BC';
								break;
						case '\x0E8':
								c = '\x0CC';
								break;
						case '\x0E9':
								c = '\x0B9';
								break;
						case '\x0EA':
								c = '\x0CE';
								break;
						case '\x0D1':
								c = '\x0F9';
								break;
						case '\x0A6':
								c = '\x0B1';
								break;
						case '\x083':
								c = '\x0F9';
								break;
						default:
								if(c>0x7E)
									c = '?';
								break;
					} // FONT_CP437

			}
			else if( has_charset(font_id, CHARSET_NUPETSCII) ){
				// c is already right
			}
			else if( font_id==FONT_ASCII ){
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
					} // ASCII & undefined

			} // test on has_charset( font_id, ... )
      handle_new_character( c );
  }
}

char read_key(){
  // read a key from input buffer (the keyboard or serial line). This is used
  // for menu handling. Return 0 if no char available
  if( key_ready()==false ) return 0;
  if(conio_config.cursor.state.blink_state)
    conio_config.cursor.state.blink_state = false; // hide cursor
  return read_key_from_buffer();
}


// === Screen based function ===================================================

void clrscr(){ // standard definition for clear screen
  for(int r=0;r<ROWS;r++){
      // tighter method, as too much of a delay here can cause dropped characters
      void *sl = &ptr[r]->slot[0];
      memset(sl, 0, COLUMNS);

      sl = &ptr[r]->inv[0];
      memset(sl, 0, COLUMNS);

      sl = &ptr[r]->blk[0];
      memset(sl, 0, COLUMNS);
  }
}

void clear_primary_screen(){
  clrscr();
}

void clear_secondary_screen(){
    for(int r=0;r<ROWS;r++){
        // tighter method, as too much of a delay here can cause dropped characters
        void *sl = &secondary_ptr[r]->slot[0];
        memset(sl, 0, COLUMNS);

        sl = &secondary_ptr[r]->inv[0];
        memset(sl, 0, COLUMNS);

        sl = &secondary_ptr[r]->blk[0];
        memset(sl, 0, COLUMNS);
    }
}

void copy_secondary_to_main_screen(){
    for(int r=0;r<ROWS;r++){
        memcpy(ptr[r]->slot,
                secondary_ptr[r]->slot,
                sizeof(ptr[r]->slot));

        memcpy(ptr[r]->inv,
                secondary_ptr[r]->inv,
                sizeof(ptr[r]->inv));

        memcpy(ptr[r]->blk,
                secondary_ptr[r]->blk,
                sizeof(ptr[r]->blk));
    }
}

void copy_main_to_secondary_screen(){
    for(int r=0;r<ROWS;r++){
        void *src = &ptr[r]->slot[0];
        void *dst = &secondary_ptr[r]->slot[0];
        memcpy(dst, src, sizeof(secondary_ptr[r]->slot));

        src = &ptr[r]->inv[0];
        dst =  &secondary_ptr[r]->inv[0];
        memcpy(dst, src, sizeof(secondary_ptr[r]->inv));

        src = &ptr[r]->blk[0];
        dst =  &secondary_ptr[r]->blk[0];
        memcpy(dst, src, sizeof(secondary_ptr[r]->blk));
    }
}

void clear_screen_from_cursor(){
    clear_line_from_cursor();
    for(int r=conio_config.cursor.pos.y+1;r<ROWS;r++){
        for(int c=0;c<COLUMNS;c++){
            put_char(0,c,r);    // todo: should use the new method in clear_entire_screen
        }
    }
}

void clear_screen_to_cursor(){
    clear_line_to_cursor();
    for(int r=0;r<conio_config.cursor.pos.y;r++){
        for(int c=0;c<COLUMNS;c++){
            put_char(0,c,r);  // todo: should use the new method in clear_entire_screen
        }
    }
}

void shuffle_down(){
    // this is our scroll
    // because we're using pointers to rows, we only need to shuffle the array of pointers

    // recycle first line.
    struct row_of_text *temphandle = ptr[0];
    //ptr[ROWS-1]=ptr[0];

    for(int r=0;r<ROWS-1;r++){
        ptr[r]=ptr[r+1];
    }

    ptr[ROWS-1] = temphandle;

    // recycled line needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[ROWS-1]->slot[i] = 0;
    ptr[ROWS-1]->inv[i] = 0;
        ptr[ROWS-1]->blk[i] = 0;
    }
}

void shuffle_up(){
    // this is our scroll
    // because we're using pointers to rows, we only need to shuffle the array of pointers

    // recycle first line.
    struct row_of_text *temphandle = ptr[ROWS-1];
    //ptr[ROWS-1]=ptr[0];

    for(int r=ROWS-2;r>=0;r--){
        ptr[r+1]=ptr[r];
    }

    ptr[0] = temphandle;

    // recycled line needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[0]->slot[i] = 0;
    ptr[0]->inv[i] = 0;
        ptr[0]->blk[i] = 0;
    }
}


// === Line based function =====================================================

void insert_line(){
    struct row_of_text *temphandle = ptr[ROWS-1];

    for(int r=ROWS-1;r>conio_config.cursor.pos.y;r--){
        ptr[r] = ptr[r-1];
    }

    ptr[conio_config.cursor.pos.y] = temphandle;

    // recycled row needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[conio_config.cursor.pos.y]->slot[i] = 0;
        ptr[conio_config.cursor.pos.y]->inv[i] = 0;
        ptr[conio_config.cursor.pos.y]->blk[i] = 0;
    }
}

void delete_line(){
    struct row_of_text *temphandle = ptr[conio_config.cursor.pos.y];

    for(int r=conio_config.cursor.pos.y;r<ROWS-1;r++){
        ptr[r]=ptr[r+1];
    }

    ptr[ROWS-1] = temphandle;

    // recycled row needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[ROWS-1]->slot[i] = 0;
        ptr[ROWS-1]->inv[i] = 0;
        ptr[ROWS-1]->blk[i] = 0;
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
    // new faster method
    void *sl = &ptr[conio_config.cursor.pos.y]->slot[conio_config.cursor.pos.x];
    memset(sl, 0, COLUMNS-conio_config.cursor.pos.x);

    sl = &ptr[conio_config.cursor.pos.y]->inv[conio_config.cursor.pos.x];
    memset(sl, 0, COLUMNS-conio_config.cursor.pos.x);

    sl = &ptr[conio_config.cursor.pos.y]->blk[conio_config.cursor.pos.x];
    memset(sl, 0, COLUMNS-conio_config.cursor.pos.x);
}

void clear_line_to_cursor(){
    void *sl = &ptr[conio_config.cursor.pos.y]->slot[0];
    memset(sl, 0, conio_config.cursor.pos.x);

    sl = &ptr[conio_config.cursor.pos.y]->inv[0];
    memset(sl, 0, conio_config.cursor.pos.x);

    sl = &ptr[conio_config.cursor.pos.y]->blk[0];
    memset(sl, 0, conio_config.cursor.pos.x);
}

void clear_entire_line(){
    void *sl = &ptr[conio_config.cursor.pos.y]->slot[0];
    memset(sl, 0, COLUMNS);

    sl = &ptr[conio_config.cursor.pos.y]->inv[0];
    memset(sl, 0, COLUMNS);

    sl = &ptr[conio_config.cursor.pos.y]->blk[0];
    memset(sl, 0, COLUMNS);
}

// === Char based function =====================================================

void set_char(int x, int y, char ch ){
  ptr[y]->slot[x] = ch;
}

void set_reverse( int x, int y, bool state ){
  /* indicates a position to be drawed in REVERSE/INVERSE video */
  ptr[y]->inv[x] = state ? 1 : 0;
}

void set_blinking( int x, int y, bool state ){
  /* indicates a position to be drawed as Blinking */
  ptr[y]->blk[x] = state ? 1 : 0;
}

unsigned char slop_character(int x,int y){
    // nb returns screen code - starts with space at zero, ie ascii-32
    //return p[y].slot[x];
    return ptr[y]->slot[x];
}

unsigned char * slotsForRow(int y){
    return &ptr[y]->slot[0];
}
unsigned char * slotsForInvRow(int y){
    return &ptr[y]->inv[0];
}
unsigned char * slotsForBlkRow(int y){
    return &ptr[y]->blk[0];
}


void delete_chars(int n){
    int c = conio_config.cursor.pos.x;
    for(int i=conio_config.cursor.pos.x + n;i<COLUMNS;i++){
        ptr[conio_config.cursor.pos.y]->slot[c] = ptr[conio_config.cursor.pos.y]->slot[i];
        ptr[conio_config.cursor.pos.y]->inv[c] = ptr[conio_config.cursor.pos.y]->inv[i];
        ptr[conio_config.cursor.pos.y]->blk[c] = ptr[conio_config.cursor.pos.y]->blk[i];
        c++;
    }
    for(int i=c;i<COLUMNS;i++){
        ptr[conio_config.cursor.pos.y]->slot[i] = 0;
        ptr[conio_config.cursor.pos.y]->inv[i] = 0;
        ptr[conio_config.cursor.pos.y]->blk[i] = 0;
    }
}

void erase_chars(int n){
    int c = conio_config.cursor.pos.x;
    for(int i=conio_config.cursor.pos.x;i<COLUMNS && i<c+n;i++){
        ptr[conio_config.cursor.pos.y]->slot[i] = 0;
        ptr[conio_config.cursor.pos.y]->inv[i] = 0;
        ptr[conio_config.cursor.pos.y]->blk[i] = 0;
    }
}

void insert_chars(int n){

    for(int r=COLUMNS-1;r>=conio_config.cursor.pos.x+n;r--){
        ptr[conio_config.cursor.pos.y]->slot[r] = ptr[conio_config.cursor.pos.y]->slot[r-n];
        ptr[conio_config.cursor.pos.y]->inv[r] = ptr[conio_config.cursor.pos.y]->inv[r-n];
        ptr[conio_config.cursor.pos.y]->blk[r] = ptr[conio_config.cursor.pos.y]->blk[r-n];
    }

    erase_chars(n);
}

unsigned char inv_character(int x,int y){
    return ptr[y]->inv[x];
}

unsigned char blk_character(int x,int y){
    return ptr[y]->blk[x];
}

void put_char(unsigned char ch,int x,int y){
    if(conio_config.cursor.pos.x>=COLUMNS || conio_config.cursor.pos.y>=VISIBLEROWS){
        return;
    }

    //decmode on DOMEU
    if(conio_config.dec_mode != DEC_MODE_NONE){
        //ch = ch + 32; // going from array_index to ASCII code
        ch = get_dec_char( config.font_id, conio_config.dec_mode, ch+32 ); // +32 to go from array_index to ASCII code
        set_char( x, y, ch-32 );
    }
    else{
        set_char(x,y,ch);
    }

  if(conio_config.rvs) // Reverse drawing
    set_reverse( x,y, true );
  else
    set_reverse( x,y, false );

  if(conio_config.blk) // blinking drawing
    set_blinking( x,y, true );
  else
    set_blinking( x,y, false );

   if (conio_config.just_wrapped)
     conio_config.just_wrapped = false;
}

// === Cursor based function ===================================================

void refresh_cursor(){
  clear_cursor();
  print_cursor();
}

void print_cursor(){
  __chr_under_csr = slop_character(conio_config.cursor.pos.x,conio_config.cursor.pos.y);
  __inv_under_csr = inv_character(conio_config.cursor.pos.x,conio_config.cursor.pos.y);
  __blk_under_csr = blk_character(conio_config.cursor.pos.x,conio_config.cursor.pos.y);

  if(conio_config.cursor.state.visible==false || (conio_config.cursor.state.blinking_mode && conio_config.cursor.state.blink_state)) return;

  if(__chr_under_csr == 0) // config.nupetscii &&
    ptr[conio_config.cursor.pos.y]->slot[conio_config.cursor.pos.x] = conio_config.cursor.symbol;

  else if(__inv_under_csr == 1)
    ptr[conio_config.cursor.pos.y]->inv[conio_config.cursor.pos.x] = 0;

  else
    ptr[conio_config.cursor.pos.y]->inv[conio_config.cursor.pos.x] = 1;
}

void clear_cursor(){
    //slip_character(chr_under_csr,csr.x,csr.y); // fix 191121
    // can't use slip, because it applies reverse
    ptr[conio_config.cursor.pos.y]->slot[conio_config.cursor.pos.x] = __chr_under_csr;
    ptr[conio_config.cursor.pos.y]->inv[conio_config.cursor.pos.x] = __inv_under_csr;
    ptr[conio_config.cursor.pos.y]->blk[conio_config.cursor.pos.x] = __blk_under_csr;
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

void move_cursor_home(){
    move_cursor_at(1, 1);
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
