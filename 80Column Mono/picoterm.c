/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 &21
 * Shiela Dixon, https://peacockmedia.software
 *
 * main.c handles the ins and outs
 * picoterm.c handles the behaviour of the terminal and storing the text
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include "picoterm.h"
#include "../common/pmhid.h"
#include "tusb_option.h"
#include <stdio.h>
#include "main.h"
#include "hardware/watchdog.h"

#define COLUMNS     80
#define ROWS        34
#define VISIBLEROWS 30
#define CSRCHAR     128


// escape sequence state
#define ESC_READY               0
#define ESC_ESC_RECEIVED        1
#define ESC_PARAMETER_READY     2

#define MAX_ESC_PARAMS          5
static int esc_state = ESC_READY;
static int esc_parameters[MAX_ESC_PARAMS];
static bool parameter_q;
static int esc_parameter_count;
static unsigned char esc_c1;
static unsigned char esc_final_byte;

bool cursor_visible;
static bool rvs = false;
static unsigned char chr_under_csr;



void make_cursor_visible(bool v){
    cursor_visible=v;
}

void clear_escape_parameters(){
    for(int i=0;i<MAX_ESC_PARAMS;i++){
        esc_parameters[i]=0;
    }
    esc_parameter_count = 0;
}

void reset_escape_sequence(){
    clear_escape_parameters();
    esc_state=ESC_READY;
    esc_c1=0;
    esc_final_byte=0;
    parameter_q=false;
}




typedef struct row_of_text { unsigned char slot[COLUMNS]; } row_of_text;
//struct row_of_text rows[ROWS];  // make 100 of our text rows
//static struct row_of_text *p = &rows[0];  // pointer p assigned the address of the first row
    // then p[y].slot[x] = ch;
    // and return p[y].slot[x];
    // and to scroll p += 1; // 1 row of text

// array of pointers, each pointer points to a row structure
static struct row_of_text *ptr[ROWS];


typedef struct point {
  int x;
  int y;
} point;

struct point csr = {0,0};
struct point saved_csr = {0,0};


void constrain_cursor_values(){
    if(csr.x<0) csr.x=0;
    if(csr.x>=COLUMNS) csr.x=COLUMNS-1;
    if(csr.y<0) csr.y=0;
    if(csr.y>=VISIBLEROWS) csr.y=VISIBLEROWS-1;
}


void slip_character(unsigned char ch,int x,int y){

    if(csr.x>=COLUMNS || csr.y>=VISIBLEROWS){
        return;
    }

    if(rvs && ch<95){   // 95 is the start of the rvs character set
        ch = ch + 95;
    }
    ptr[y]->slot[x] = ch;
}

unsigned char slop_character(int x,int y){
    // nb returns screen code - starts with space at zero, ie ascii-32
    //return p[y].slot[x];
    return ptr[y]->slot[x];
}

unsigned char * slotsForRow(int y){
    return &ptr[y]->slot[0];
}

/*
    ptr[ROWS-1] = ptr[ROWS-2];
    ptr[ROWS-2] = ptr[ROWS-3];
    // ...
    ptr[csr.y+1] = ptr[csr.y];
*/



void insert_line(){

    struct row_of_text *temphandle = ptr[ROWS-1];

    for(int r=ROWS-1;r>csr.y;r--){
        ptr[r] = ptr[r-1];
    }

    ptr[csr.y] = temphandle;

    // recycled row needs blanking
    for(int i=0;i<COLUMNS;i++){
        ptr[csr.y]->slot[i] = 0;
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




void shuffle(){
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
    }
}

void print_cursor(){
    chr_under_csr = slop_character(csr.x,csr.y);
    if(cursor_visible==false) return;

    unsigned char rvs_chr = chr_under_csr;
    if(rvs_chr>=95){        // yes, 95, our screen codes start at ascii 0x20-0x7f
        rvs_chr -= 95;
    }
    else{
       rvs_chr += 95;
    }
    //slip_character(rvs_chr,csr.x,csr.y); // fix 191121
    // can't use slip, because it applies reverse
    ptr[csr.y]->slot[csr.x] = rvs_chr;
}
void clear_cursor(){
    //slip_character(chr_under_csr,csr.x,csr.y); // fix 191121
    // can't use slip, because it applies reverse
    ptr[csr.y]->slot[csr.x] = chr_under_csr;
}


void clear_line_from_cursor(){
    //for(int c=csr.x;c<COLUMNS;c++){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[csr.x];
    memset(sl, 0, COLUMNS-csr.x);


}
void clear_line_to_cursor(){
    //for(int c=csr.x;c>=0;c--){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, csr.x);

}
void clear_entire_line(){
    //for(int c=0;c<COLUMNS;c++){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, COLUMNS);

}


void clear_entire_screen(){

    for(int r=0;r<ROWS;r++){
        //slip_character(0,c,r);
        // tighter method, as too much of a delay here can cause dropped characters
        void *sl = &ptr[r]->slot[0];
        memset(sl, 0, COLUMNS);

    }
}

void clear_screen_from_csr(){
    clear_line_from_cursor();
    for(int r=csr.y;r<ROWS;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);    // todo: should use the new method in clear_entire_screen
        }
    }
}

void clear_screen_to_csr(){
    clear_line_to_cursor();
    for(int r=0;r<csr.y;r++){
        for(int c=0;c<COLUMNS;c++){
            slip_character(0,c,r);  // todo: should use the new method in clear_entire_screen
        }
    }
}


// for debugging purposes only
void print_ascii_value(unsigned char asc){
    // takes value eg 65 ('A') and sends characters '6' and '5' (0x36 and 0x35)
    int hundreds = asc/100;
    unsigned char remainder = asc-(hundreds*100);
    int tens = remainder/10;
    remainder = remainder-(tens*10);
    if(hundreds>0){
        handle_new_character(0x30+hundreds);
    }
    if(tens>0 || hundreds>0){
        handle_new_character(0x30+tens);
    }
    handle_new_character(0x30+remainder);
    handle_new_character(' ');
    if(csr.x>COLUMNS-5){
        handle_new_character(CR);
        handle_new_character(LF);
    }
}


void esc_sequence_received(){
/*
// these should now be populated:
    static int esc_parameters[MAX_ESC_PARAMS];
    static int esc_parameter_count;
    static unsigned char esc_c1;
    static unsigned char esc_final_byte;
*/


int n,m;
if(esc_c1=='['){
    // CSI
    switch(esc_final_byte){
    case 'H':
        // Moves the cursor to row n, column m
        // The values are 1-based, and default to 1

        n = esc_parameters[0];
        m = esc_parameters[1];
        n--;
        m--;

        // these are zero based
        csr.x = m;
        csr.y = n;
        constrain_cursor_values();
    break;

    case 'h':
        if(parameter_q && esc_parameters[0]==25){
            // show csr
            make_cursor_visible(true);
        }
    break;
    case 'l':
        if(parameter_q && esc_parameters[0]==25){
            // hide csr
            make_cursor_visible(false);
        }
    break;


    case 'm':
        //SGR
        // Sets colors and style of the characters following this code
        //TODO: allows multiple paramters
        switch(esc_parameters[0]){
            case 0:
            // reset / normal
            rvs = false;
        break;
            case 7:
            rvs = true;
        break;
        }
    break;

    case 's':
        // save cursor position
        saved_csr.x = csr.x;
        saved_csr.y = csr.y;
    break;
    case 'u':
        // move to saved cursor position
        csr.x = saved_csr.x;
        csr.y = saved_csr.y;
    break;

    case 'J':
    // Clears part of the screen. If n is 0 (or missing), clear from cursor to end of screen.
    // If n is 1, clear from cursor to beginning of the screen. If n is 2, clear entire screen
    // (and moves cursor to upper left on DOS ANSI.SYS).
    // If n is 3, clear entire screen and delete all lines saved in the scrollback buffer
    // (this feature was added for xterm and is supported by other terminal applications).
        switch(esc_parameters[0]){
            case 0:
            // clear from cursor to end of screen
            clear_screen_from_csr();
        break;
            case 1:
            // clear from cursor to beginning of the screen
            clear_screen_to_csr();
        break;
            case 2:
            // clear entire screen
            clear_entire_screen();
            csr.x=0; csr.y=0;
        break;
        case 3:
            // clear entire screen
            clear_entire_screen();
            csr.x=0; csr.y=0;
        break;
        }

    break;




    case 'K':
    // Erases part of the line. If n is 0 (or missing), clear from cursor to the end of the line.
    // If n is 1, clear from cursor to beginning of the line. If n is 2, clear entire line.
    // Cursor position does not change.
        switch(esc_parameters[0]){
            case 0:
            // clear from cursor to the end of the line
            clear_line_from_cursor();
        break;
            case 1:
            // clear from cursor to beginning of the line
            clear_line_to_cursor();
        break;
            case 2:
            // clear entire line
            clear_entire_line();
        break;
        }
    break;


    case 'A':
    // Cursor Up
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.y -= n;
        constrain_cursor_values();
    break;
    case 'B':
    // Cursor Down
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.y += n;
        constrain_cursor_values();  // todo: should possibly do a scroll up?
    break;
    case 'C':
    // Cursor Forward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.x += n;
        constrain_cursor_values();
    break;
    case 'D':
    // Cursor Backward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.x -= n;
        constrain_cursor_values();
    break;

    case 'S':
    // Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
        n = esc_parameters[0];
        if(n==0)n=1;
        for(int i=0;i<n;i++){
            shuffle();
        }
    break;

    // MORE



    case 'L':
    // 'INSERT LINE' - scroll rows down from and including cursor position. (blank the cursor's row??)
        n = esc_parameters[0];
        if(n==0)n=1;
        insert_lines(n);
    break;

    case 'M':
    // 'DELETE LINE' - delete row at cursor position, scrolling everything below, up to fill. Leaving blank line at bottom.
        n = esc_parameters[0];
        if(n==0)n=1;
        delete_lines(n);
    break;


    }

}
else{
    // ignore everything else
}


// our work here is done
reset_escape_sequence();

}


void prepare_text_buffer(){

    reset_escape_sequence();

    for(int c=0;c<ROWS;c++){
        struct row_of_text *newRow;
        /* Create structure in memory */
        newRow=(struct row_of_text *)malloc(sizeof(struct row_of_text));
        if(newRow==NULL)
        {
            exit(1);
        }
        ptr[c] = newRow;
    }

    // print cursor
    make_cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

void display_terminal(){
		// do we need to blank them, ie fill with 0?
    char msg[80];

		clear_entire_screen();
		csr.x = 0; csr.y = 0;

		print_string("_/_/_/_/_/_/     _/_/_/_/_/     _/_/_/_/_/     _/_/_/_/_/     _/_/     _/_/  _/\r\n");
		print_string("_/_/_/_/_/_/     _/_/_/_/_/     _/_/_/_/_/     _/_/_/_/_/     _/_/     _/_/  _/\r\n");
		print_string("_/_/      _/_/ _/_/      _/_/ _/_/      _/_/ _/_/    _/_/_/ _/_/_/     _/_/  _/\r\n");
		print_string("_/_/      _/_/ _/_/      _/_/ _/_/      _/_/ _/_/    _/_/_/ _/_/_/     _/_/  _/\r\n");
		print_string("_/_/      _/_/ _/_/                   _/_/   _/_/  _/  _/_/   _/_/     _/_/  _/\r\n");
		print_string("_/_/      _/_/ _/_/                   _/_/   _/_/  _/  _/_/   _/_/   _/_/    _/\r\n");
		print_string("_/_/_/_/_/_/   _/_/             _/_/_/_/     _/_/  _/  _/_/   _/_/   _/_/    _/\r\n");
		print_string("_/_/_/_/_/_/   _/_/             _/_/_/_/     _/_/  _/  _/_/   _/_/   _/_/    _/\r\n");
		print_string("_/_/      _/_/ _/_/           _/_/           _/_/  _/  _/_/   _/_/   _/_/_/_/_/\r\n");
		print_string("_/_/      _/_/ _/_/           _/_/           _/_/  _/  _/_/   _/_/   _/_/_/_/_/\r\n");
		print_string("_/_/      _/_/ _/_/      _/_/ _/_/      _/_/ _/_/_/    _/_/   _/_/         _/_/\r\n");
		print_string("_/_/      _/_/ _/_/      _/_/ _/_/      _/_/ _/_/_/    _/_/   _/_/         _/_/\r\n");
		print_string("_/_/      _/_/   _/_/_/_/_/   _/_/_/_/_/_/_/   _/_/_/_/_/ _/_/_/_/_/_/     _/_/\r\n");
		print_string("_/_/      _/_/   _/_/_/_/_/   _/_/_/_/_/_/_/   _/_/_/_/_/ _/_/_/_/_/_/     _/_/\r\n");
		print_string("                                                        Menu : CTRL+SHIF+M\r\n");
		sprintf(msg, "\r\nTinyUSB=%d.%d.%d, ", TUSB_VERSION_MAJOR, TUSB_VERSION_MINOR,TUSB_VERSION_REVISION);
		print_string(msg);
		sprintf(msg, "Keymap=%s rev %d\r\n", KEYMAP, KEYMAP_REV ); // , Menu toggle: CTRL+SHIFT+M
		print_string(msg);
		// Update "project(picoterm VERSION 1.1)" in CMakeList
		sprintf(msg, "PicoTerm %s  S. Dixon\r\n", CMAKE_PROJECT_VERSION );
		print_string(msg);


    // print cursor
    make_cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}


void display_menu(){
    // do we need to blank them, ie fill with 0?
    char msg[40];

    reset_escape_sequence();
    clear_entire_screen();
    csr.x = 0; csr.y = 0;

    //print_string("================================================================================\r\n");
    print_string("                          >>>>  PicoTerm Menu <<<<\r\n");
		print_string("\r\n");
		print_string("+- Terminal Colors (reboot) -+\r\n");
		print_string("|   0 WHITE        3 GREEN1  |\r\n" );
		print_string("|   1 LIGHT_AMBER  4 GREEN2  |\r\n" );
		print_string("|   2 DARK_AMBER   5 GREEN3  |\r\n" );
		print_string("+----------------------------+\r\n" );
		print_string("\r\nSelect an option: ");


    make_cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}


void print_string(char str[]){
    for(int i=0;i<strlen(str);i++){
        handle_new_character(str[i]);
    }
}

char read_key(){
  // read a key from input buffer (the keyboard or serial line). This is used
  // for menu handling. Return 0 if no char available
  if( key_ready()==false )
    return 0;
  return read_key_from_buffer();
}

char handle_menu_input(){
  // check if user selected an option THEN execute the appropriate action
  // return the pressed key if any.
  char _ch = read_key();
  if( (_ch==0) || (_ch==ESC) )
    return _ch;

  // display char on terminal
  clear_cursor();
  handle_new_character(_ch);
  print_cursor();

  if( (_ch >= '0') && (_ch <= '5') ) {
		uint8_t _color = _ch - 48; // 48->53 to 0->5 (WHITE->GREEN3)
		stop_core1(); // suspend rendering for race condition
		sleep_ms(10);
		colour_preference = _color;
    write_data_to_flash();
		render_on_core1();
		print_string( "\r\nWrite to flash! Will reboot in 2 seconds.");
		watchdog_enable( 2000, 0 );
    /*print_string( "Yo! MAN!\r\n");
		read_data_from_flash();
		char msg[40];
		sprintf( msg, "pref: %i\r\n", colour_preference );
		print_string( msg );*/
	}

  return _ch;
}

void handle_new_character(unsigned char asc){

    // handle escape sequences
    if(esc_state != ESC_READY){
        switch(esc_state){
            case ESC_ESC_RECEIVED:
                // waiting on c1 character
                if(asc>='N' && asc<'_'){
                    // 0x9B = CSI, that's the only one we're interested in atm
                    // the others are 'Fe Escape sequences'
                    // usually two bytes, ie we have them already.
                    if(asc=='['){    // ESC+[ =  0x9B){
                        // move forward

                        esc_c1 = asc;
                        esc_state=ESC_PARAMETER_READY;
                        clear_escape_parameters();
                    }
                    // other type Fe sequences go here
                    else{
                        // for now, do nothing
                        reset_escape_sequence();
                    }
                }
                else{
                    // unrecognised character after escape.
                    reset_escape_sequence();
                }
                break;
            case ESC_PARAMETER_READY:
                // waiting on parameter character, semicolon or final byte
                if(asc>='0' && asc<='9'){
                    // parameter value
                    if(esc_parameter_count<MAX_ESC_PARAMS){
                        unsigned char digit_value = asc - 0x30; // '0'
                        esc_parameters[esc_parameter_count] *= 10;
                        esc_parameters[esc_parameter_count] += digit_value;
                    }

                }
                else if(asc==';'){
                    // move to next param
                    esc_parameter_count++;
                    if(esc_parameter_count>MAX_ESC_PARAMS) esc_parameter_count=MAX_ESC_PARAMS;
                }
                else if(asc=='?'){
                    parameter_q=true;
                }
                else if(asc>=0x40 && asc<0x7E){
                    // final byte. Log and handle
                    esc_final_byte = asc;
                    esc_sequence_received();
                }
                else{
                    // unexpected value, undefined
                }
                break;
        }




    }
    else{
        // regular characters -
        if(asc>=0x20 && asc<0x7f){

            slip_character(asc-32,csr.x,csr.y);
            csr.x++;


            // this for disabling wrapping in terminal
            constrain_cursor_values();

            // alternatively, use this code for enabling wrapping in terminal
            // NB the last released source has a bug - add the -1 after VISIBLEROWS to fix, if you really want to turn the wrapping on.
            /*
            if(csr.x>=COLUMNS){
                csr.x=0;
                if(csr.y==VISIBLEROWS-1){
                    shuffle();
                }
                else{
                    csr.y++;
                }
            }
            */


        }
        //is it esc?
        else if(asc==0x1B){
            esc_state=ESC_ESC_RECEIVED;
        }
        else{
            // return, backspace etc
            switch (asc){
                case BSP:
                if(csr.x>0){
                    csr.x--;
                }
                break;
                case LF:

                    if(csr.y==VISIBLEROWS-1){   // visiblerows is the count, csr is zero based
                        shuffle();
                    }
                    else{
                    csr.y++;
                    }
                break;
                case CR:
                    csr.x=0;

                break;
                case FF:
                    clear_entire_screen();
                    csr.x=0; csr.y=0;

                break;
            }

        }

    } // not esc sequence



}
