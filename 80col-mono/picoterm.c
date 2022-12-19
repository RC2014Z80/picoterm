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
#include "../common/picoterm_config.h"
#include "tusb_option.h"
#include <stdio.h>
#include "main.h"
#include "hardware/watchdog.h"
#include "../common/picoterm_debug.h"

#define COLUMNS     80
#define ROWS        34
#define VISIBLEROWS 30
#define CSRCHAR     128

// wrap text
#define WRAP_TEXT

// escape sequence state
#define ESC_READY               0
#define ESC_ESC_RECEIVED        1
#define ESC_PARAMETER_READY     2

#define MAX_ESC_PARAMS          5
static int esc_state = ESC_READY;
static int esc_parameters[MAX_ESC_PARAMS];
static bool parameter_q;
static bool parameter_p;
static bool parameter_sp;
static int esc_parameter_count;
static unsigned char esc_c1;
static unsigned char esc_final_byte;

bool cursor_visible;
bool cursor_blinking = false;
bool cursor_blinking_mode = true;
char cursor_symbol = 143;

bool dec_mode = false;
int dec_mode_type = 0;

bool insert_mode = false;

bool wrap_text = true;
//#ifdef	WRAP_TEXT
bool just_wrapped = false;
//#endif

static bool rvs = false;
static bool blk = false;
static unsigned char chr_under_csr;
static bool inv_under_csr;
static bool blk_under_csr;

extern picoterm_config_t config; // Issue #13, awesome contribution of Spock64

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
    parameter_p=false;
    parameter_sp=false;
}




typedef struct row_of_text {
	unsigned char slot[COLUMNS];
	unsigned char inv[COLUMNS];
    unsigned char blk[COLUMNS];
} row_of_text;
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
	/*
    if(rvs && ch<95){   // 95 is the start of the rvs character set
        ch = ch + 95;
    }
	*/

    //decmode on
    if(dec_mode){
        ch = ch + 32;
        if(ch >= 'j' && ch <= 'x')
        {
            if(dec_mode_type == 1){
                switch(ch){
                case 'j':    //0x6a	j	+
                    ptr[y]->slot[x] = 0xe7;
                break;
                case 'k':    //0x6b	k	+
                    ptr[y]->slot[x] = 0xe4;
                break;
                case 'l':    //0x6c	l	+
                    ptr[y]->slot[x] = 0xe2;
                break;
                case 'm':    //0x6d	m	+
                    ptr[y]->slot[x] = 0xe5;
                break;
                case 'n':    //0x6e	n	+
                    ptr[y]->slot[x] = 0xea;
                break;
                case 'q':    //0x71	q	-
                    ptr[y]->slot[x] = 0xe1;
                break;
                case 't':    //0x74	t	+
                    ptr[y]->slot[x] = 0xe8;
                break;
                case 'u':    //0x75	u	¦
                    ptr[y]->slot[x] = 0xe9;
                break;
                case 'v':    //0x76	v	-
                    ptr[y]->slot[x] = 0xe6;
                break;
                case 'w':    //0x77	w	-
                    ptr[y]->slot[x] = 0xe3;
                break;
                case 'x':    //0x78	x	¦
                    ptr[y]->slot[x] = 0xe0;
                break;
                default:
                    ptr[y]->slot[x] = ch;
                break;
                }
            }
            else{
                switch(ch){
                case 'j':    //0x6a	j	+
                    ptr[y]->slot[x] = 0xbd;
                break;
                case 'k':    //0x6b	k	+
                    ptr[y]->slot[x] = 0xae;
                break;
                case 'l':    //0x6c	l	+
                    ptr[y]->slot[x] = 0xb0;
                break;
                case 'm':    //0x6d	m	+
                    ptr[y]->slot[x] = 0xad;
                break;
                case 'n':    //0x6e	n	+
                    ptr[y]->slot[x] = 0xdb;
                break;
                case 'q':    //0x71	q	-
                    ptr[y]->slot[x] = 0xc3;
                break;
                case 't':    //0x74	t	+
                    ptr[y]->slot[x] = 0xab;
                break;
                case 'u':    //0x75	u	¦
                    ptr[y]->slot[x] = 0xb3;
                break;
                case 'v':    //0x76	v	-
                    ptr[y]->slot[x] = 0xb1;
                break;
                case 'w':    //0x77	w	-
                    ptr[y]->slot[x] = 0xb2;
                break;
                case 'x':    //0x78	x	¦
                    ptr[y]->slot[x] = 0xdd;
                break;
                default:
                    ptr[y]->slot[x] = ch;
                break;
                }
            }
        }
        else{
            ptr[y]->slot[x] = ch;
        }
        ptr[y]->slot[x] = ptr[y]->slot[x] - 32;
    }
    else{
        ptr[y]->slot[x] = ch;
    }
    
	if(rvs)
		ptr[y]->inv[x] = 1;
	else
		ptr[y]->inv[x] = 0;

	if(blk)
		ptr[y]->blk[x] = 1;
	else
		ptr[y]->blk[x] = 0;        

//#ifdef	WRAP_TEXT
 	if (just_wrapped) just_wrapped = false;
//#endif
}

unsigned char slop_character(int x,int y){
    // nb returns screen code - starts with space at zero, ie ascii-32
    //return p[y].slot[x];
    return ptr[y]->slot[x];
}

unsigned char inv_character(int x,int y){
    return ptr[y]->inv[x];
}

unsigned char blk_character(int x,int y){
    return ptr[y]->blk[x];
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



void wrap_constrain_cursor_values(){

	if(csr.x>=COLUMNS) {
		csr.x=0;
		if(csr.y==VISIBLEROWS-1){   // visiblerows is the count, csr is zero based
			shuffle_down();
		}
		else{
			csr.y++;
		}
//#ifdef	WRAP_TEXT
		just_wrapped = true;
//#endif
	}
}

bool get_csr_blink_state() { return cursor_blinking; }
void set_csr_blink_state(bool state) { cursor_blinking = state; }

void refresh_cursor(){
	clear_cursor();
	print_cursor();
}

void print_cursor(){

    chr_under_csr = slop_character(csr.x,csr.y);
	inv_under_csr = inv_character(csr.x,csr.y);
    blk_under_csr = blk_character(csr.x,csr.y);

    if(cursor_visible==false || (cursor_blinking_mode && cursor_blinking)) return;

	if(config.nupetscii && chr_under_csr == 0)
		ptr[csr.y]->slot[csr.x] = cursor_symbol;

	else if(inv_under_csr == 1)
		ptr[csr.y]->inv[csr.x] = 0;

	else
		ptr[csr.y]->inv[csr.x] = 1;

	/*
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
	*/
}


void clear_cursor(){
    //slip_character(chr_under_csr,csr.x,csr.y); // fix 191121
    // can't use slip, because it applies reverse
    ptr[csr.y]->slot[csr.x] = chr_under_csr;
	ptr[csr.y]->inv[csr.x] = inv_under_csr;
    ptr[csr.y]->blk[csr.x] = blk_under_csr;
}


void clear_line_from_cursor(){
    //for(int c=csr.x;c<COLUMNS;c++){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[csr.x];
    memset(sl, 0, COLUMNS-csr.x);

	sl = &ptr[csr.y]->inv[csr.x];
    memset(sl, 0, COLUMNS-csr.x);

	sl = &ptr[csr.y]->blk[csr.x];
    memset(sl, 0, COLUMNS-csr.x);

}
void clear_line_to_cursor(){
    //for(int c=csr.x;c>=0;c--){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, csr.x);

	sl = &ptr[csr.y]->inv[0];
    memset(sl, 0, csr.x);

	sl = &ptr[csr.y]->blk[0];
    memset(sl, 0, csr.x);    

}
void clear_entire_line(){
    //for(int c=0;c<COLUMNS;c++){
    //    slip_character(0,c,csr.y);
    //}
    // new faster method
    void *sl = &ptr[csr.y]->slot[0];
    memset(sl, 0, COLUMNS);

	sl = &ptr[csr.y]->inv[0];
    memset(sl, 0, COLUMNS);

	sl = &ptr[csr.y]->blk[0];
    memset(sl, 0, COLUMNS);    
}


void clear_entire_screen(){

    for(int r=0;r<ROWS;r++){
        //slip_character(0,c,r);
        // tighter method, as too much of a delay here can cause dropped characters
        void *sl = &ptr[r]->slot[0];
        memset(sl, 0, COLUMNS);

		sl = &ptr[r]->inv[0];
        memset(sl, 0, COLUMNS);

		sl = &ptr[r]->blk[0];
        memset(sl, 0, COLUMNS);        
    }
}

void clear_screen_from_csr(){
    clear_line_from_cursor();
    for(int r=csr.y+1;r<ROWS;r++){
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

//ESC H           Set tab at current column
//ESC [ g         Clear tab at current column
//ESC [ 0g        Same
//ESC [ 3g        Clear all tabs

//ESC H	HTS	Horizontal Tab Set	Sets a tab stop in the current column the cursor is in.
//ESC [ <n> I	CHT	Cursor Horizontal (Forward) Tab	Advance the cursor to the next column (in the same row) with a tab stop. If there are no more tab stops, move to the last column in the row. If the cursor is in the last column, move to the first column of the next row.
//ESC [ <n> Z	CBT	Cursor Backwards Tab	Move the cursor to the previous column (in the same row) with a tab stop. If there are no more tab stops, moves the cursor to the first column. If the cursor is in the first column, doesn’t move the cursor.
//ESC [ 0 g	TBC	Tab Clear (current column)	Clears the tab stop in the current column, if there is one. Otherwise does nothing.
//ESC [ 3 g	TBC	Tab Clear (all columns)

int n,m;
if(esc_c1=='['){
    // CSI
    switch(esc_final_byte){
    case 'H':
	case 'f':
        // Moves the cursor to row n, column m
        // The values are 1-based, and default to 1

        //Move cursor to upper left corner ESC [H
        //Move cursor to upper left corner ESC [;H
        //Move cursor to screen location v,h ESC [<v>;<h>H
        //Move cursor to upper left corner ESC [f
        //Move cursor to upper left corner ESC [;f
        //Move cursor to screen location v,h ESC [<v>;<h>f

        n = esc_parameters[0];
        m = esc_parameters[1];
        n--;
        m--;

        // these are zero based
        csr.x = m;
        csr.y = n;
        constrain_cursor_values();
    break;

	case 'E':
        // ESC[#E	moves cursor to beginning of next line, # lines down

		n = esc_parameters[0];
		if(n==0)n=1;

		// these are zero based
        csr.x = 0;
        csr.y += n;
        constrain_cursor_values();
    break;

	case 'F':
        // ESC[#F	moves cursor to beginning of previous line, # lines up

		n = esc_parameters[0];
		if(n==0)n=1;

		// these are zero based
        csr.x = 0;
        csr.y -= n;
        constrain_cursor_values();
    break;


	case 'd':
        // ESC[#d	moves cursor to an absolute # line

		n = esc_parameters[0];
		n--;

		// these are zero based
        csr.y = n;
        constrain_cursor_values();
    break;

	case 'G':
        // ESC[#G	moves cursor to column #

		n = esc_parameters[0];
		n--;

		// these are zero based
        csr.x = n;
        constrain_cursor_values();
    break;

    case 'h':
        //[ 2 h		Keyboard locked
        //[ 4 h		Insert mode selected
        //[ 20 h    Set new line mode

        //[ ? 1 h       Set cursor key to application
        //[ ? 2 h       Set ANSI (versus VT52)
        //[ ? 3 h		132 Characters on
        //[ ? 4 h		Smooth Scroll on
        //[ ? 5 h		Inverse video on
        //[ ? 7 h		Wraparound ON
        //[ ? 8 h		Autorepeat ON
        //[ ? 9 h       Set 24 lines per screen (default) 
        //[ ? 12 h		Text Cursor Enable Blinking
        //[ ? 14 h  	Immediate operation of ENTER key
        //[ ? 16 h  	Edit selection immediate
        //[ ? 25 h  	Cursor ON
        //[ ? 50 h  	Cursor ON
        //[ ? 75 h  	Screen display ON

        if(parameter_q){ 
            if(esc_parameters[0]==25 || esc_parameters[0]==50){
                // show csr
                make_cursor_visible(true);
            }
            else if(esc_parameters[0]==7){
                //Auto-wrap mode on (default) ESC [?7h
                wrap_text = true;
            }
            else if(esc_parameters[0]==12){
                //Text Cursor Enable Blinking
                cursor_blinking_mode = true;
            }
        }
        else{
            if(esc_parameters[0]==4){
                //Insert mode selected
                insert_mode = true;
            }
        }
    break;
    case 'l':
        //[ 2 l		Keyboard unlocked
        //[ 4 l		Replacement mode selected
        //[ 20 l    Set line feed mode

        //[ ? 1 l       Set cursor key to cursor               
        //[ ? 2 l       Set VT52 (versus ANSI)                 
        //[ ? 3 l		80 Characters on
        //[ ? 4 l		Jump Scroll on
        //[ ? 5 l		Normal video off
        //[ ? 7 l		Wraparound OFF
        //[ ? 8 l		Autorepeat OFF
        //[ ? 9 l       Set 36 lines per screen 
        //[ ? 12 l	    Text Cursor Disable Blinking
        //[ ? 14 l	    Deferred operation of ENTER key
        //[ ? 16 l	    Edit selection deferred
        //[ ? 25 l	    Cursor OFF
        //[ ? 50 l	    Cursor OFF
        //[ ? 75 l	    Screen display OFF
        if(parameter_q){
            if(esc_parameters[0]==25 || esc_parameters[0]==50){
                // hide csr
                make_cursor_visible(false);
            }
            else if(esc_parameters[0]==7){
                //Auto-wrap mode off ESC [?7l
                wrap_text = false;
            }
            else if(esc_parameters[0]==12){
                //Text Cursor Disable Blinking
                cursor_blinking_mode = false;
            }
        }
        else{
            if(esc_parameters[0]==4){
                //Replacement mode selected
                insert_mode = false;
            }
        }
    break;


    case 'm':
        //SGR
        // Sets colors and style of the characters following this code
        //TODO: allows multiple parameters        
        //[ 0 m		Clear all character attributes
        //[ 1 m		(Bold) Alternate Intensity ON
        //[ 3 m     Select font #2 (large characters)
        //[ 4 m		Underline ON
        //[ 5 m		Blink ON
        //[ 6 m     Select font #2 (jumbo characters)
        //[ 7 m		Inverse video ON
        //[ 8 m     Turn invisible text mode on
        //[ 22 m		Alternate Intensity OFF
        //[ 24 m		Underline OFF
        //[ 25 m		Blink OFF
        //[ 27 m		Inverse Video OFF
        if(esc_parameters[0]==0){
            rvs = false; // reset / normal
            blk = false;
        }
        else if(esc_parameters[0]==5){
            blk = true;
        }
        else if(esc_parameters[0]==7){
            rvs = true;
        }
        else if(esc_parameters[0]==25){
            blk = false;
        }        
        else if(esc_parameters[0]==27){
            rvs = false;
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
            shuffle_down();
        }
    break;

    case 'T':
    // Scroll whole page down by n (default 1) lines. New lines are added at the top. (not ANSI.SYS)
        n = esc_parameters[0];
        if(n==0)n=1;
        for(int i=0;i<n;i++){
            shuffle_up();
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

    case 'P':
    // 'DELETE CHARS' - delete <n> characters at the current cursor position, shifting in space characters from the right edge of the screen.
        n = esc_parameters[0];
        if(n==0)n=1;
        delete_chars(n);
    break;

    case 'X':
    // 'ERASE CHARS' - erase <n> characters from the current cursor position by overwriting them with a space character.
        n = esc_parameters[0];
        if(n==0)n=1;
        erase_chars(n);
    break;    

    case '@':
    // 'Insert Character' - insert <n> spaces at the current cursor position, shifting all existing text to the right. Text exiting the screen to the right is removed.
        n = esc_parameters[0];
        if(n==0)n=1;
        insert_chars(n);
    break;    

    case 'q':

        if(parameter_sp){
            parameter_sp = false;
            
            //ESC [ 0 SP q	DECSCUSR	User Shape	Default cursor shape configured by the user
            //ESC [ 1 SP q	DECSCUSR	Blinking Block	Blinking block cursor shape
            //ESC [ 2 SP q	DECSCUSR	Steady Block	Steady block cursor shape
            //ESC [ 3 SP q	DECSCUSR	Blinking Underline	Blinking underline cursor shape
            //ESC [ 4 SP q	DECSCUSR	Steady Underline	Steady underline cursor shape
            //ESC [ 5 SP q	DECSCUSR	Blinking Bar	Blinking bar cursor shape
            //ESC [ 6 SP q	DECSCUSR	Steady Bar	Steady bar cursor shape

            switch(esc_parameters[0]){
                case 0:
                cursor_symbol = 143;
                cursor_blinking_mode = true;
            break;
                case 1:
                cursor_symbol = 121; //95;
                cursor_blinking_mode = true;
            break;
                case 2:
                cursor_symbol = 121; //95;
                cursor_blinking_mode = false;
            break;
                case 3:
                cursor_symbol = 143;
                cursor_blinking_mode = true;
            break;
                case 4:
                cursor_symbol = 143;
                cursor_blinking_mode = false;
            break;
                case 5:
                cursor_symbol = 148;
                cursor_blinking_mode = true;
            break;
                case 6:
                cursor_symbol = 148;
                cursor_blinking_mode = false;
            break;                
            }
        }
    break;
    }

}
else if(esc_c1=='('){
    // CSI
    switch(esc_final_byte){
    case 'B':
        dec_mode = false;
    break;
    case '0':
        dec_mode = true;
        dec_mode_type = 0;
    break;
    case '2':
        dec_mode = true;
        dec_mode_type = 1;
    break;    
    default:
        dec_mode = false;
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
    print_string("    S.Dixon & D.Meurisse                               Help : CTRL+SHIFT+H\r\n");
    sprintf(msg, "\r\nTinyUSB=%d.%d.%d, ", TUSB_VERSION_MAJOR, TUSB_VERSION_MINOR,TUSB_VERSION_REVISION);
    print_string(msg);
    sprintf(msg, "Keymap=%s rev %d\r\n", KEYMAP, KEYMAP_REV ); // , Menu toggle: CTRL+SHIFT+M
    print_string(msg);
    char _parity = '?';
    switch(config.parity){
      case UART_PARITY_NONE:
        _parity = 'N';
        break;
      case UART_PARITY_ODD:
        _parity = 'O';
        break;
      case UART_PARITY_EVEN:
        _parity = 'E';
        break;
    }
    // Update "project(picoterm VERSION 1.1)" in CMakeList
    sprintf(msg, "PicoTerm %s @ %i bds %i%c%i\r\n", CMAKE_PROJECT_VERSION, config.baudrate, config.databits, _parity, config.stopbits );
    print_string(msg);


    // print cursor
    make_cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}


void display_config(){
    char msg[80];
    reset_escape_sequence();
    clear_entire_screen();
    csr.x = 0; csr.y = 0;

    __print_string("\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 PicoTerm Menu \x0A6\x0A6\r\n" , !(config.nupetscii) ); // strip Nupetscii when not activated
    __print_string("\x0E2\x0E1 Terminal Colors \x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E4\r\n", !(config.nupetscii) );
    sprintf(msg, "\x0E0  %s0 White   %s1 Light Amber %s2 Dark Amber   \x0E0\r\n", (config.colour_preference==0)?"\x0D1":" ", (config.colour_preference==1)?"\x0D1":" ", (config.colour_preference==2)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf( msg, "\x0E0  %s3 Green1  %s4 Green2      %s5 Green3       \x0E0\r\n", (config.colour_preference==3)?"\x0D1":" ", (config.colour_preference==4)?"\x0D1":" ", (config.colour_preference==5)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf( msg, "\x0E0  %s6 Purple                                 \x0E0\r\n", (config.colour_preference==6)?"\x0D1":" " );
    __print_string( msg, !(config.nupetscii) );
    __print_string("\x0E8\x0C3 Serial  Baud \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B2\x0C3 Data bit \x0C3\x0C3\x0E9\r\n", !(config.nupetscii) );
    sprintf(msg, "\x0E0  %sa 115200 %sb 57600 %sc 38400 \x0C2 %s8  8 bits  \x0E0\r\n", (config.baudrate==115200)?"\x0D1":" " , (config.baudrate==57600)?"\x0D1":" ", (config.baudrate==38400)?"\x0D1":" ", (config.databits==8)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf(msg, "\x0E0  %sd 19200  %se 9600  %sf 4800  \x0C2 %s7  7 bits  \x0E0\r\n", (config.baudrate==19200)?"\x0D1":" " , (config.baudrate==9600)?"\x0D1":" ", (config.baudrate==4800)?"\x0D1":" ", (config.databits==7)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf(msg, "\x0E0  %sg 2400   %sh 1200  %si 300   \x0C2             \x0E0\r\n", (config.baudrate==2400)?"\x0D1":" " , (config.baudrate==1200)?"\x0D1":" " , (config.baudrate==300)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    __print_string("\x0E8\x0C3 Parity \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0DB\x0C3 Stop bits \x0C3\x0E9\r\n", !(config.nupetscii) );
    sprintf(msg, "\x0E0  %sn None   %so Odd   %sv Even  \x0C2 %sw  1 bit   \x0E0\r\n", (config.parity==UART_PARITY_NONE)?"\xD1":" " , (config.parity==UART_PARITY_ODD)?"\xD1":" ", (config.parity==UART_PARITY_EVEN)?"\xD1":" " , (config.stopbits==1)?"\xD1":" "  );
    __print_string(msg, !(config.nupetscii) );
    sprintf(msg, "\x0E8\x0C3 Charset \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3 %sx  2 bits  \x0E0\r\n", (config.stopbits==2)?"\xD1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf(msg, "\x0E0   %sl VT100 (7bits+reverse)   \x0C2             \x0E0\r\n", (config.nupetscii==0)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    sprintf(msg, "\x0E0   %sm NupetSCII (8bits)       \x0C2             \x0E0\r\n", (config.nupetscii==1)?"\x0D1":" " );
    __print_string(msg, !(config.nupetscii) );
    __print_string("\x0E5\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x08A\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E7\r\n", !(config.nupetscii) );
    print_string("\r\n(S upcase=save / ESC=close) ? ");


    make_cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

int replace_char(char *str, char orig, char rep) {
    char *ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

void display_nupetscii(){
  char msg[80];
  char _c;
  reset_escape_sequence();
  clear_entire_screen();
  csr.x = 0; csr.y = 0;

  __print_string( "\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 NuPETSCII Charset \x0A6\x0A6\r\n" , !(config.nupetscii) ); // strip Nupetscii when not activated
  print_string( "     0 1 2 3 4 5 6 7 8 9 A B C D E F\r\n");
  __print_string( "   \x0B0\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0AE\r\n"  , !(config.nupetscii) );
  for( char line=2; line<16; line++ ){
    sprintf( msg, "%02X \x0C2 ", line*16 );
    __print_string( msg , !(config.nupetscii) ); // strip Nupetscii when not activated
    for( char index=0; index<=15; index++ ){
      _c = line*16+index;
      sprintf( msg, "%c ", _c );
      print_string( msg );
    }
    __print_string("\x0C2\r\n", !(config.nupetscii) );
    // Insert a index line in the middle for easier reading
    if( line==8 ){
      __print_string( "   \x0AB\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3\r\n", !(config.nupetscii) );
      __print_string( "   \x0C2 0 1 2 3 4 5 6 7 8 9 A B C D E F \x0C2\r\n" , !(config.nupetscii) );
      __print_string( "   \x0AB\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3\r\n", !(config.nupetscii) );
    }
  }
  __print_string( "   \x0AD\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0BD\r\n", !(config.nupetscii) );
  print_string( "     0 1 2 3 4 5 6 7 8 9 A B C D E F\r\n");

  print_string("\r\n(ESC=close) ? ");
  make_cursor_visible(true);
  clear_cursor();  // so we have the character
  print_cursor();  // turns on
}


void display_help(){
  char msg[80];
  char _c;
  reset_escape_sequence();
  clear_entire_screen();
  csr.x = 0; csr.y = 0;
  __print_string("\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 PicoTerm Help \x0A6\x0A6\r\n", !(config.nupetscii) );
  __print_string("\x0B0\x0C3 Keyboard Shortcut \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0AE\r\n", !(config.nupetscii) );
  __print_string("\x0C2 \x083 Shift+Ctrl+H : Help screen                   \x0C2\r\n", !(config.nupetscii) ); // strip Nupetscii when not activated
  __print_string("\x0C2 \x083 Shift+Ctrl+L : Toggle NupetSCII/VT100 charset\x0C2\r\n", !(config.nupetscii) );
  __print_string("\x0C2 \x083 Shift+Ctrl+M : Configuration menu            \x0C2\r\n", !(config.nupetscii) );
  __print_string("\x0C2 \x083 Shift+Ctrl+N : Display NupetScii charset     \x0C2\r\n", !(config.nupetscii) );
  __print_string("\x0C2                                                \x0C2\r\n", !(config.nupetscii) );
  __print_string("\x0AD\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0BD\r\n", !(config.nupetscii));

  print_string("\r\n(ESC=close) ? ");
  make_cursor_visible(true);
  clear_cursor();  // so we have the character
  print_cursor();  // turns on
}

void print_string(char str[] ){
  __print_string( str, false );
}

void __print_string(char str[], bool strip_nupetscii ){
   // remove the NuPetScii extended charset from a string and replace them with
  // more convenient.
  // This function is used by the configuration screen. See display_config().
  char c;
  for(int i=0;i<strlen(str);i++){
      c = str[i];
      if( strip_nupetscii )
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

char read_key(){
  // read a key from input buffer (the keyboard or serial line). This is used
  // for menu handling. Return 0 if no char available
  if( key_ready()==false ) return 0;

  if(cursor_blinking) cursor_blinking = false;

  return read_key_from_buffer();
}

char handle_default_input(){
  // Make your own specialized menu input handler (if needed, see handle_menu_input)
  // and call it as needed from main.c::main()
  char _ch = read_key();
  return _ch;
}

char handle_config_input(){
  // check if user selected an option THEN execute the appropriate action
  // return the pressed key if any.
  char _ch = read_key();
  if( (_ch==0) || (_ch==ESC) )
    return _ch;
  //sprintf( debug_msg, "readkey %c", _ch ); // Do not work... dont know why
  //debug_print( debug_msg ); // proceed asap by main loop

  // display char on terminal
  clear_cursor();
  handle_new_character(_ch);
  print_cursor();

  // Store the config
  if ( _ch == 'S' ){
    print_string( "\r\nWrite to flash! Will reboot in 2 seconds.");
    sleep_ms( 1000 );
    stop_core1(); // suspend rendering for race condition
    sleep_ms(10);
    save_config();
    watchdog_enable( 1000, 0 );
  }

  // change the color
  if( (_ch >= '0') && (_ch <= '6') ) {
    uint8_t _color = _ch - 48; // 48->54 to 0->6 (WHITE->PURPLE)
    config.colour_preference = _color;
    build_font( config.nupetscii ); // rebuilt font
  }

  // Baud rate
  if( ( _ch >= 'a') && (_ch <= 'i') ) {
    switch( _ch ){
      case 'a':
        config.baudrate = 115200;
        break;
      case 'b':
        config.baudrate = 57600;
        break;
	  case 'c':
        config.baudrate = 38400;
        break;
      case 'd':
        config.baudrate = 19200;
        break;
      case 'e':
        config.baudrate = 9600;
        break;
      case 'f':
        config.baudrate = 4800;
        break;
      case 'g':
        config.baudrate = 2400;
        break;
      case 'h':
        config.baudrate = 1200;
        break;
      case 'i':
        config.baudrate = 300;
        break;
    }
    uart_set_baudrate( UART_ID, config.baudrate );
    display_config();
  }
  // data bit configuration
  if ( ( _ch >= '7') && (_ch <= '8') ) {
    switch( _ch ){
      case '7':
        config.databits = 7;
        break;
      case '8':
        config.databits = 8;
        break;
    }
    uart_set_format(UART_ID, config.databits, config.stopbits, config.parity );
    display_config();
  }
  // Stop bit configuration
  if ( ( _ch >= 'w') && (_ch <= 'x') ) {
    switch( _ch ){
      case 'w':
        config.stopbits = 1;
        break;
      case 'x':
        config.stopbits = 2;
        break;
    }
    uart_set_format(UART_ID, config.databits, config.stopbits, config.parity );
    display_config();
  }
  // Parity configuration
  if ( ( _ch >= 'n') || (_ch <= 'o') || (_ch <= 'v')) {
    switch( _ch ){
      case 'n':
        config.parity = UART_PARITY_NONE;
        break;
      case 'o':
        config.parity = UART_PARITY_ODD;
        break;
      case 'v':
        config.parity = UART_PARITY_EVEN;
        break;
    }
    uart_set_format(UART_ID, config.databits, config.stopbits, config.parity );
    display_config();
  }
  // NupetSCII font configuration
  if ( ( _ch >= 'l') && (_ch <= 'm') ) {
    switch( _ch ){
      case 'l':
        config.nupetscii = 0; // normal font
        break;
      case 'm':
        config.nupetscii = 1;
        break;
    }
    build_font(config.nupetscii==1);
    display_config();
  }

  return _ch;
}

void handle_new_character(unsigned char asc){
  if(esc_state != ESC_READY){
      // === ESC SEQUENCE ====================================================
      switch(esc_state){
          case ESC_ESC_RECEIVED:
              // --- waiting on c1 character ---
              // c1 is the first parameter after the ESC
              if(asc>='N' && asc<'_'){
                  // 0x9B = CSI, that's the only one we're interested in atm
                  // the others are 'Fe Escape sequences'
                  // usually two bytes, ie we have them already.
                  if(asc=='['){    // ESC+[ =  0x9B){
                      esc_c1 = asc;
                      esc_state=ESC_PARAMETER_READY; // Lets wait for parameter
                      clear_escape_parameters();
                  }
                  // other type Fe sequences go here
                  else
                      // for now, do nothing
                      reset_escape_sequence();
              }
              else if ( asc=='F' ){
                  config.nupetscii=1; // Enter graphic charset
                  build_font( true );
                  reset_escape_sequence();
              }
              else if (asc=='G'){
                  config.nupetscii=0; // Enter ASCII charset
                  build_font( false );
                  reset_escape_sequence();
              }
              else
                  // unrecognised character after escape.
                  reset_escape_sequence();
              break;
          case ESC_PARAMETER_READY:
              // waiting on parameter character, semicolon or final byte
              if(asc>='0' && asc<='9'){

                  if(parameter_p){
                    // final byte. Log and handle
                    esc_final_byte = asc;
                    esc_sequence_received();                    
                  }
                  else{
                    // parameter value
                    if(esc_parameter_count<MAX_ESC_PARAMS){
                        unsigned char digit_value = asc - 0x30; // '0'
                        esc_parameters[esc_parameter_count] *= 10;
                        esc_parameters[esc_parameter_count] += digit_value;
                    }
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
              else if(asc==' '){
                  parameter_sp=true;
              }
              else if(asc=='('){
                  esc_c1 = '(';
                  parameter_p=true;
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
  else {
      // === regular characters ==============================================
      if(asc>=0x20 && asc<=0xFF){
          
          //if insert mode shift chars to the right
          if(insert_mode) insert_chars(1);

          // --- Strict ASCII <0x7f or Extended NuPetSCII <= 0xFF ---
          slip_character(asc-32,csr.x,csr.y);
          csr.x++;

          if(!wrap_text){
//#ifndef	WRAP_TEXT
          // this for disabling wrapping in terminal
          constrain_cursor_values();
//#endif
          }
          else{
//#ifdef	WRAP_TEXT
          // alternatively, use this code for enabling wrapping in terminal
          wrap_constrain_cursor_values();
//#endif
          }

      }
      else if(asc==0x1B){
          // --- Begin of ESCAPE SEQUENCE ---
          esc_state=ESC_ESC_RECEIVED;
      }
      else {
          // --- return, backspace etc ---
          switch (asc){
              case BSP:
                if(csr.x>0){
                  csr.x--;
                }
                break;

              case LF:
                if(wrap_text){
    //#ifdef	WRAP_TEXT
                    if(!just_wrapped){
    //#endif
                    if(csr.y==VISIBLEROWS-1){ // visiblerows is the count, csr is zero based
                        shuffle_down();
                    }
                    else {
                        csr.y++;
                    }
    //#ifdef	WRAP_TEXT
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
                break;

              case CR:
                csr.x=0;
                break;

              case FF:
                clear_entire_screen();
                csr.x=0; csr.y=0;
                break;
          } // switch(asc)
      } // else
  } // eof Regular character

  if(cursor_blinking) cursor_blinking = false;

}
