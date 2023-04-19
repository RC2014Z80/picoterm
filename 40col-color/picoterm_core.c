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

#include "picoterm_core.h"
#include "picoterm_conio.h"


/* picoterm_conio.c */
extern picoterm_conio_config_t conio_config;
extern uint16_t foreground_colour;
extern uint16_t background_colour;


char bell_state = 0;

// early declaration
void reset_escape_sequence();

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


void terminal_init(){
	reset_escape_sequence();
	conio_init( palette[15], PICO_SCANVIDEO_PIXEL_FROM_RGB8(68,77,142) ); // foreground / Background color
	cursor_visible(true);
	clear_cursor(); // make as 80 columns²
	print_cursor();  // turns on
}

void terminal_reset(){
    move_cursor_home();
    reset_saved_cursor();

    //mode = VT100;
    //insert_mode = false;

    conio_reset();
    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

char get_bell_state() { return bell_state; }
void set_bell_state(char state) { bell_state = state; }


void reset_escape_sequence(){
    clear_escape_parameters();
    esc_state=ESC_READY;
    esc_c1=0;
    esc_final_byte=0;
    parameter_q=false;
}


void clear_escape_parameters(){
    for(int i=0;i<MAX_ESC_PARAMS;i++){
        esc_parameters[i]=0;
    }
    esc_parameter_count = 0;
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
				if(n == 0) n = 1;
				if(m == 0) m = 1;
				move_cursor_at( n, m );
    		break;

    case 'h':
        if(parameter_q && esc_parameters[0]==25){
            // show csr
            cursor_visible(true);
        }
    break;
    case 'l':
        if(parameter_q && esc_parameters[0]==25){
            // hide csr
            cursor_visible(false);
        }
    break;


    case 'm':
        //SGR
        // Sets colors and style of the characters following this code
        //TODO: allows multiple paramters
        if(esc_parameters[0]==0){
            conio_config.rvs = false;
        }
        if(esc_parameters[0]==7){
            conio_config.rvs = true;
        }
        if(esc_parameters[0]>=30 && esc_parameters[0]<=37){
            foreground_colour = palette[esc_parameters[0]-30];
        }
        if(esc_parameters[0]>=40 && esc_parameters[0]<=47){
            foreground_colour = palette[esc_parameters[0]-40];
        }
        if(esc_parameters[0]>=90 && esc_parameters[0]<=97){
            foreground_colour = palette[esc_parameters[0]-82]; // 90 is palette[8]
        }
        if(esc_parameters[0]>=100 && esc_parameters[0]<=107){
            foreground_colour = palette[esc_parameters[0]-92];  // 100 is palette[8]
        }

        //case 38:
        //Next arguments are 5;n or 2;r;g;b
        if(esc_parameters[0]==38 && esc_parameters[1]==5){
            if(esc_parameters[2]>=0 && esc_parameters[2]<=15){
                foreground_colour = palette[esc_parameters[2]];
            }
            if(esc_parameters[2]>=16 && esc_parameters[2]<=231){
                // 16-231:  6 × 6 × 6 cube (216 colors): 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)

                int cube = esc_parameters[2]-16;
                int r = cube/36;
                cube -= (r*36);
                int g = cube/6;
                cube -= (g*36);
                int b = cube;

                foreground_colour = PICO_SCANVIDEO_PIXEL_FROM_RGB8(r*42,g*42,b*42);

            }
            if(esc_parameters[2]>=232 && esc_parameters[2]<=255){
                // grayscale from black to white in 24 steps
                int gre = esc_parameters[2]-232; // 0-24
                gre *= 10.6;
                foreground_colour=PICO_SCANVIDEO_PIXEL_FROM_RGB8(gre,gre,gre);
            }


        }
        if(esc_parameters[0]==48 && esc_parameters[1]==5){
            if(esc_parameters[2]>=0 && esc_parameters[2]<=15){
                background_colour = palette[esc_parameters[2]];
            }
            if(esc_parameters[2]>=16 && esc_parameters[2]<=231){
                // 16-231:  6 × 6 × 6 cube (216 colors): 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)

                int cube = esc_parameters[2]-16;
                int r = cube/36;
                cube -= (r*36);
                int g = cube/6;
                cube -= (g*36);
                int b = cube;

                background_colour = PICO_SCANVIDEO_PIXEL_FROM_RGB8(r*42,g*42,b*42);
            }
            if(esc_parameters[2]>=232 && esc_parameters[2]<=255){
                // grayscale from black to white in 24 steps
                int gre = esc_parameters[2]-232; // 0-24
                gre *= 10.6;
                background_colour=PICO_SCANVIDEO_PIXEL_FROM_RGB8(gre,gre,gre);
            }
        }
        //Next arguments are 5;n or 2;r;g;b
        if(esc_parameters[0]==38 && esc_parameters[1]==2){
            // 2,3,4 = r,g,b
            foreground_colour=PICO_SCANVIDEO_PIXEL_FROM_RGB8(esc_parameters[2],esc_parameters[3],esc_parameters[4]);
        }
        if(esc_parameters[0]==48 && esc_parameters[1]==2){
            // 2,3,4 = r,g,b
            background_colour=PICO_SCANVIDEO_PIXEL_FROM_RGB8(esc_parameters[2],esc_parameters[3],esc_parameters[4]);
        }

    break;

    case 's':
        // save cursor position
				save_cursor_position();
        //saved_csr.x = csr.x;
        //saved_csr.y = csr.y;
    break;
    case 'u':
        // move to saved cursor position
				restore_cursor_position();
        //csr.x = saved_csr.x;
        //csr.y = saved_csr.y;
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
            clear_screen_from_cursor();
        break;
            case 1:
            // clear from cursor to beginning of the screen
            clear_screen_to_cursor();
        break;
            case 2:
            // clear entire screen
            clear_primary_screen();
            //csr.x=0; csr.y=0;
						move_cursor_home();
        break;
        case 3:
            // clear entire screen
            clear_primary_screen();
            //csr.x=0; csr.y=0;
						move_cursor_home();
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
        conio_config.cursor.pos.y -= n;
        constrain_cursor_values();
    break;
    case 'B':
    // Cursor Down
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        conio_config.cursor.pos.y += n;
        constrain_cursor_values();  // todo: should possibly do a scroll up?
    break;
    case 'C':
    // Cursor Forward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        conio_config.cursor.pos.x += n;
        constrain_cursor_values();
    break;
    case 'D':
    // Cursor Backward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        conio_config.cursor.pos.x -= n;
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

    // MORE


     case 'U':
        if(parameter_q){
            // user-defined character data follows
            // first parameter is the character number, which must be 128-255 inclusive
            // follow with 8 bytes which will be inserted into the UD character space

            data_purpose=UDCHAR;
            current_udchar = esc_parameters[0];
            data_bytes_expected = 8;
        }
    		break;

    }
	}
	else{
    // ignore everything else
	}

	// our work here is done
	reset_escape_sequence();
}

void handle_udchar_data(uint8_t d){

    data_bytes_expected--;
    // doing this first makes it 7 - 0
    static int bytenum;
    bytenum = 7-data_bytes_expected;
    //to make 0-7
    bytenum+=((current_udchar-128)*8);

    custom_bitmap[bytenum] = d;
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
    // data?
        if(data_bytes_expected>0){
            if(data_purpose==ARBITRARY){

            }
            if(data_purpose==BITMAPDATA){

            }
            if(data_purpose==UDCHAR){
                handle_udchar_data((uint8_t)asc);
            }
        }
        else{
            // regular characters -

            if(asc>=0x20 && asc<=0xff){

                slip_character(asc-32,conio_config.cursor.pos.x,conio_config.cursor.pos.y);
                conio_config.cursor.pos.x++;


                // this for disabling / enabling wrapping in terminal
                #ifndef LINEWRAP
                    constrain_cursor_values();
                #else
                    // this code for enabling wrapping in terminal
                    if(conio_config.cursor.pos.x>=COLUMNS){
                        conio_config.cursor.pos.x=0;
                        if(conio_config.cursor.pos.y==TEXTROWS-1){
													  // scroll the whole screen page UP
                            shuffle_down();
                        }
                        else{
                            conio_config.cursor.pos.y++;
                        }
                    }
                #endif
            }
            //is it esc?
            else if(asc==0x1B){
                esc_state=ESC_ESC_RECEIVED;
            }
            else{
                // return, backspace etc
                switch (asc){
                    case BSP:
                    if(conio_config.cursor.pos.x>0){
                        conio_config.cursor.pos.x--;
                    }
                    break;
                    case LF:

                        if(conio_config.cursor.pos.y==TEXTROWS-1){   // visiblerows is the count, csr is zero based
                            shuffle_down();
                        }
                        else{
                        conio_config.cursor.pos.y++;
                        }
                    break;
                    case CR:
                        conio_config.cursor.pos.x=0;

                    break;
                    case FF:
                        clear_primary_screen();
												move_cursor_home();
                        //conio_config.cursor.pos.x=0; conio_config.cursor.pos.y=0;

                    break;
                }

            }
        }

    } // not esc sequence
}


/* see picoterm_conio::print_element()
void print_logo_element(int x,int scanlineNumber){
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
} */

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
    if(conio_config.cursor.pos.x>COLUMNS-5){
        handle_new_character(CR);
        handle_new_character(LF);
    }
}
