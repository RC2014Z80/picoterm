/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading key from keyboard, etc
   ========================================================================== */

#include <stdbool.h>
#include <string.h>
#include "picoterm_conio.h"
#include "../common/picoterm_cursor.h"
#include "../common/picoterm_dec.h"
#include "../common/picoterm_stddef.h"
#include "../common/keybd.h" // Keyboard device
#include "picoterm.h"
#include "../common/picoterm_config.h"

/* picoterm_config.c */

/* picoterm_cursor.c */
extern bool cursor_blinking;

/* picoterm_cursor.c */
extern point_t csr;
extern point_t saved_csr;
extern bool cursor_blinking;
extern bool cursor_blinking_mode;
extern bool cursor_visible;
extern char cursor_symbol;

/* picoterm_dec.c */
extern uint8_t dec_mode;

/* picoterm_config.c */
extern picoterm_config_t config;

array_of_row_text_pointer ptr;           // primary screen content
array_of_row_text_pointer secondary_ptr; // secondary screen content

//#ifdef  WRAP_TEXT
bool just_wrapped = false;
//#endif
bool wrap_text = true; // terminal configured to warp_text around

bool rvs = false;
bool blk = false;
unsigned char chr_under_csr;
bool inv_under_csr;
bool blk_under_csr;

void conio_init(){
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
}

void conio_reset( char default_cursor_symbol ){
  // Reset the terminal
  rvs = false;
  blk = false;

  chr_under_csr = 0;
  inv_under_csr = 0;
  blk_under_csr = 0;

  cursor_visible = true;
  cursor_blinking = false;
  cursor_blinking_mode = true;
  cursor_symbol = default_cursor_symbol;

  wrap_text = true;
  just_wrapped = false;

  dec_mode = DEC_MODE_NONE; // single/double lines
  clrscr();
  clear_secondary_screen();
}


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
    for(int r=csr.y+1;r<ROWS;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);    // todo: should use the new method in clear_entire_screen
        }
    }
}

void clear_screen_to_cursor(){
    clear_line_to_cursor();
    for(int r=0;r<csr.y;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);  // todo: should use the new method in clear_entire_screen
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

    for(int r=ROWS-1;r>csr.y;r--){
        ptr[r] = ptr[r-1];
    }

    ptr[csr.y] = temphandle;

    // recycled row needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[csr.y]->slot[i] = 0;
        ptr[csr.y]->inv[i] = 0;
        ptr[csr.y]->blk[i] = 0;
    }
}

void delete_line(){
    struct row_of_text *temphandle = ptr[csr.y];

    for(int r=csr.y;r<ROWS-1;r++){
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
    void *sl = &ptr[csr.y]->slot[csr.x];
    memset(sl, 0, COLUMNS-csr.x);

    sl = &ptr[csr.y]->inv[csr.x];
    memset(sl, 0, COLUMNS-csr.x);

    sl = &ptr[csr.y]->blk[csr.x];
    memset(sl, 0, COLUMNS-csr.x);
}

void clear_line_to_cursor(){
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, csr.x);

    sl = &ptr[csr.y]->inv[0];
    memset(sl, 0, csr.x);

    sl = &ptr[csr.y]->blk[0];
    memset(sl, 0, csr.x);
}

void clear_entire_line(){
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, COLUMNS);

    sl = &ptr[csr.y]->inv[0];
    memset(sl, 0, COLUMNS);

    sl = &ptr[csr.y]->blk[0];
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
    int c = csr.x;
    for(int i=csr.x + n;i<COLUMNS;i++){
        ptr[csr.y]->slot[c] = ptr[csr.y]->slot[i];
        ptr[csr.y]->inv[c] = ptr[csr.y]->inv[i];
        ptr[csr.y]->blk[c] = ptr[csr.y]->blk[i];
        c++;
    }
    for(int i=c;i<COLUMNS;i++){
        ptr[csr.y]->slot[i] = 0;
        ptr[csr.y]->inv[i] = 0;
        ptr[csr.y]->blk[i] = 0;
    }
}

void erase_chars(int n){
    int c = csr.x;
    for(int i=csr.x;i<COLUMNS && i<c+n;i++){
        ptr[csr.y]->slot[i] = 0;
        ptr[csr.y]->inv[i] = 0;
        ptr[csr.y]->blk[i] = 0;
    }
}

void insert_chars(int n){

    for(int r=COLUMNS-1;r>=csr.x+n;r--){
        ptr[csr.y]->slot[r] = ptr[csr.y]->slot[r-n];
        ptr[csr.y]->inv[r] = ptr[csr.y]->inv[r-n];
        ptr[csr.y]->blk[r] = ptr[csr.y]->blk[r-n];
    }

    erase_chars(n);
}

unsigned char inv_character(int x,int y){
    return ptr[y]->inv[x];
}

unsigned char blk_character(int x,int y){
    return ptr[y]->blk[x];
}

void slip_character(unsigned char ch,int x,int y){
    if(csr.x>=COLUMNS || csr.y>=VISIBLEROWS){
        return;
    }

    //decmode on DOMEU
    if(dec_mode != DEC_MODE_NONE){
        //ch = ch + 32; // going from array_index to ASCII code
        ch = get_dec_char( config.font_id, dec_mode, ch+32 ); // +32 to go from array_index to ASCII code
        set_char( x, y, ch-32 );
    }
    else{
        set_char(x,y,ch);
    }

  if(rvs) // Reverse drawing
    set_reverse( x,y, true );
  else
    set_reverse( x,y, false );

  if(blk) // blinking drawing
    set_blinking( x,y, true );
  else
    set_blinking( x,y, false );

//#ifdef  WRAP_TEXT
   if (just_wrapped) just_wrapped = false;
//#endif
}

// === Cursor based function ===================================================

void refresh_cursor(){
  clear_cursor();
  print_cursor();
}

void print_cursor(){
  chr_under_csr = slop_character(csr.x,csr.y);
  inv_under_csr = inv_character(csr.x,csr.y);
  blk_under_csr = blk_character(csr.x,csr.y);

    if(cursor_visible==false || (cursor_blinking_mode && cursor_blinking)) return;

  if(chr_under_csr == 0) // config.nupetscii &&
    ptr[csr.y]->slot[csr.x] = cursor_symbol;

  else if(inv_under_csr == 1)
    ptr[csr.y]->inv[csr.x] = 0;

  else
    ptr[csr.y]->inv[csr.x] = 1;
}

void clear_cursor(){
    //slip_character(chr_under_csr,csr.x,csr.y); // fix 191121
    // can't use slip, because it applies reverse
    ptr[csr.y]->slot[csr.x] = chr_under_csr;
    ptr[csr.y]->inv[csr.x] = inv_under_csr;
    ptr[csr.y]->blk[csr.x] = blk_under_csr;
}

void wrap_constrain_cursor_values(){
  if(csr.x>=COLUMNS) {
    csr.x=0;
    if(csr.y==VISIBLEROWS-1){   // visiblerows is the count, csr is zero based
      shuffle_down();
    }
    else{
      csr.y++;
    }
//#ifdef  WRAP_TEXT
    just_wrapped = true;
//#endif
  }
}

void constrain_cursor_values(){
    if(csr.x<0) csr.x=0;
    if(csr.x>=COLUMNS) csr.x=COLUMNS-1;
    if(csr.y<0) csr.y=0;
    if(csr.y>=VISIBLEROWS) csr.y=VISIBLEROWS-1;
}

void cursor_reverse_lf(){
  if(csr.y > 0)
      move_cursor_at(csr.y, csr.x + 1);
  else
      shuffle_up();
}

void move_cursor_lf( bool reverse ){
  if( reverse ){ // move cursor up
    cursor_reverse_lf();
    return;
  }

  // move cursor down
  if(wrap_text){
 //#ifdef  WRAP_TEXT
     if(!just_wrapped){
//#endif
     if(csr.y==VISIBLEROWS-1){ // visiblerows is the count, csr is zero based
         shuffle_down();
     }
     else {
         csr.y++;
     }
//#ifdef  WRAP_TEXT
     }
     else
     just_wrapped = false;
//#endif
 }
 else{
     if(csr.y==VISIBLEROWS-1){ // visiblerows is the count, csr is zero based
         shuffle_down();
     }
     else {
         csr.y++;
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
    csr.x = x;
    csr.y = y;
    constrain_cursor_values();
}

void move_cursor_up(int n){
    if(n==0)n=1;
    csr.y -= n;
    constrain_cursor_values();
}

void move_cursor_down(int n){
    if(n==0)n=1;
    csr.y += n;
    constrain_cursor_values();  // todo: should possibly do a scroll up?
}

void move_cursor_forward(int n){
    if(n==0)n=1;
    csr.x += n;
    constrain_cursor_values();
}

void move_cursor_backward(int n){
    if(n==0)n=1;
    csr.x -= n;
    constrain_cursor_values();
}

void move_cursor_home(){
    move_cursor_at(1, 1);
}
