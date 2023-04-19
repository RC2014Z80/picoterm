/* ==========================================================================
        Manage the console interaction for the PicoTerm software
              * printing string
              * reading key from keyboard, etc
   ========================================================================== */

#include <stdbool.h>
#include <string.h>
#include "picoterm_conio.h"
#include "picoterm_core.h" // scanline functions
#include "../common/keybd.h"
#include "../common/picoterm_dec.h" // DEC codification
#include "stdlib.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"

#include "../common/picoterm_debug.h"

picoterm_conio_config_t conio_config  = { .rvs = false, .blk = false, .just_wrapped = false,
    .wrap_text = true, .dec_mode = DEC_MODE_NONE, .cursor.pos.x = 0, .cursor.pos.y = 0,
    .cursor.state.visible = true, .cursor.state.blink_state = false,
    .cursor.state.blinking_mode = true, .cursor.symbol = 143 };

// Current color
uint16_t foreground_colour;
uint16_t background_colour;

// saved cursor
struct point __saved_csr = {0,0};

typedef struct scanline { uint16_t pixels[(COLUMNS*8)]; } scanline;
static struct scanline *ptr[ROWS];

//static uint16_t cursor_buffer[64] = {0};
uint16_t  __chr_under_csr[64] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
																0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
																0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
																0,0,0,0};
bool __has_chr_under_csr = false; // indicates is __chr_under_csr is supposed to contains a char (so not all black or bg_color)


void conio_init( uint16_t fg_color, uint16_t bg_color ){
	foreground_colour = fg_color;
	background_colour = bg_color;
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
  conio_config.rvs = false;
  conio_config.blk = false;
  conio_config.wrap_text = true;
  conio_config.just_wrapped = false;
  conio_config.dec_mode = DEC_MODE_NONE; // single/double lines
  // initialized @ init()
  // conio_config.ansi_font_id = FONT_NUPETSCII; // selected font_id for graphical operation

  conio_config.cursor.state.visible = true;
  conio_config.cursor.state.blink_state = false; // blinking cursor is in hidden state
  conio_config.cursor.state.blinking_mode = true;
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


void print_string(char str[]){
	// Move it to CONIO
    for(int i=0;i<strlen(str);i++){
        handle_new_character(str[i]);
    }
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

void slip_character(unsigned char ch,int x,int y){
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
                }
            }
        }
}


void clear_scanline_from_cursor(int r){
    uint16_t *sl = &ptr[r]->pixels[conio_config.cursor.pos.x*8];
    for(int i=conio_config.cursor.pos.x*8;i<COLUMNS*8;i++){
        *sl++ = background_colour;
    }
}

void clear_scanline_to_cursor(int r){

    uint16_t *sl = &ptr[r]->pixels[0];
    for(int i=0;i<conio_config.cursor.pos.x*8;i++){
        *sl++ = background_colour;
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
  /* From 80Col
	for(int r=0;r<ROWS;r++){
      // tighter method, as too much of a delay here can cause dropped characters
      void *sl = &ptr[r]->slot[0];
      memset(sl, 0, COLUMNS);

      sl = &ptr[r]->inv[0];
      memset(sl, 0, COLUMNS);

      sl = &ptr[r]->blk[0];
      memset(sl, 0, COLUMNS);
  } */
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
    /* for(int r=conio_config.cursor.pos.y+1;r<ROWS;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);    // todo: should use the new method in clear_entire_screen
        }
    } */
		// clear_text_row_from_cursor();
    for(int r=((conio_config.cursor.pos.y+1)*8);r<TEXTROWS*8;r++){
        clear_entire_scanline(r);
    }
}

void clear_screen_to_cursor(){
    clear_line_to_cursor();
    /* for(int r=0;r<conio_config.cursor.pos.y;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);  // todo: should use the new method in clear_entire_screen
        }
    }*/
		for(int r=((conio_config.cursor.pos.y+1)*8);r<TEXTROWS*8;r++){
				clear_entire_scanline(r);
		}
}

void shuffle_down(){
    // this is our scroll
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

// === Line based function =====================================================

void clear_line_from_cursor(){
    for(int r=(conio_config.cursor.pos.y*8);r<(conio_config.cursor.pos.y*8)+8;r++){
        clear_scanline_from_cursor(r);
    }
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



// === Cursor based function ===================================================
void refresh_cursor(){
  clear_cursor();
  print_cursor();
}

/* void print_cursor(){
    // these are the same
    if(conio_config.cursor.state.visible==false) return;
		//debug_print("print_cursor");
    int scanlineNumber;
    int characterPosition;
    int cursor_buffer_counter = 0;
		uint16_t newPixel;
		uint16_t pixel;
    for(int r=0;r<8;r++){   // r is a row offset
        scanlineNumber = (conio_config.cursor.pos.y*8)+r;
        characterPosition = (conio_config.cursor.pos.x*8);
        for(int bit=0;bit<8;bit++){

						//if( conio_config.cursor.state.blink_state ){
							pixel = ptr[scanlineNumber]->pixels[characterPosition+bit];
							//  Domeu: Line removed because pixel could be different from foreground or background color
							//         it is the case at boot up and it avoids the cursor to blink on the screen at boot up.
							newPixel = pixel==background_colour ? foreground_colour : background_colour;
							//newPixel = foreground_colour;
							//debug_write("X");
							cursor_buffer[cursor_buffer_counter++]=pixel;
						//} else {
						///	newPixel = background_colour;
							//debug_write("_");
						//}
						ptr[scanlineNumber]->pixels[characterPosition+bit] = newPixel;

        }
    }
		//debug_print("-"); // ensure a carriage return
}
*/
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
/*
void clear_cursor(){
    if(conio_config.cursor.state.visible==false) return;

    // put back what's in the cursor_buffer
    int scanlineNumber;
    int characterPosition;
    int cursor_buffer_counter = 0;
    for(int r=0;r<8;r++){   // r is a row offset
        scanlineNumber = (conio_config.cursor.pos.y*8)+r;
        characterPosition = (conio_config.cursor.pos.x*8);
        for(int bit=0;bit<8;bit++){
            uint16_t newPixel = cursor_buffer[cursor_buffer_counter++];
            ptr[scanlineNumber]->pixels[characterPosition+bit] = newPixel;

        }
    }
}
*/

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

void cursor_visible(bool v){
    conio_config.cursor.state.visible=v;
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
