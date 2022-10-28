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
#include "tusb_option.h"
#include "../common/picoterm_config.h"
#include "../common/pmhid.h"
#include "main.h"
#include "hardware/watchdog.h"

#define LINEWRAP        // comment out to disable line wrapping

#define COLUMNS     40
#define ROWS        256 // yes this is 16 more than the number of scanlines we have
                        // for scrolling to work we need 8 spare pointers
#define TEXTROWS    29 // ROWS/8  // must be 2 text rows less than the max number of scanlines
#define CSRCHAR     128

#define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BSP         0x08
#define LF          0x0a
#define CR          0x0d
#define FF          0x0c



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

// pixel data
#define ARBITRARY 0
#define BITMAPDATA 1
#define UDCHAR 2

static int data_purpose;
static int current_udchar;
static uint32_t data_bytes_expected;


static uint16_t palette[] = {
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0),        //black
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0, 0),     // red
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0xAA, 0),     //green
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0x55, 0),  //brown
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0xAA),     //blue
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0, 0xAA),  // magenta
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0xAA, 0xAA),  // cyan
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xAA, 0xAA, 0xAA),//light grey
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0x55, 0x55),//grey
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x55, 0x55),//bright red
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0xFF, 0x55),//bright green
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0x55),//yellow
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0xFF),      //bright blue
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0x55, 0xFF),//bright magenta
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0x55, 0xFF, 0xFF),//bright cyan
                PICO_SCANVIDEO_PIXEL_FROM_RGB8(0xFF, 0xFF, 0xFF),//white
};

static uint8_t custom_bitmap[768] = {
                0b11111000,
				0b10001100,
				0b10001100,
				0b10001100,
				0b11111100,
				0b01111100
                };

static uint16_t foreground_colour;
static uint16_t background_colour;

static uint16_t cursor_buffer[64] = {0};

bool cursor_visible;
static bool rvs = false;


void make_cursor_visible(bool v){
    cursor_visible=v;
    if(v){
        print_cursor(); // should store what's under
    }
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


typedef struct scanline { uint16_t pixels[(COLUMNS*8)]; } scanline;

static struct scanline *ptr[ROWS];


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
    if(csr.y>=TEXTROWS) csr.y=TEXTROWS-1;
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
                        if(!rvs){
                            ptr[scanlineNumber]->pixels[characterPosition+bit]
                                = foreground_colour;
                        }
                        else{
                            ptr[scanlineNumber]->pixels[characterPosition+bit]
                            = background_colour;
                        }

                    }
                    else{
                        if(rvs){
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


        //foreground_colour++;
        //if(foreground_colour==16) foreground_colour=1;

}

// most important accessor
uint32_t * wordsForRow(int y){
    return (uint32_t * )&ptr[y]->pixels[0];
}



void clear_scanline_from_cursor(int r){

    uint16_t *sl = &ptr[r]->pixels[csr.x*8];
    for(int i=csr.x*8;i<COLUMNS*8;i++){
        *sl++ = background_colour;
    }

}
void clear_scanline_to_cursor(int r){

    uint16_t *sl = &ptr[r]->pixels[0];
    for(int i=0;i<csr.x*8;i++){
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

void clear_entire_text_row(int y){
    for(int r=(y*8);r<(y*8)+8;r++){
        clear_entire_scanline(r);
    }
}

void clear_text_row_from_cursor(){
    for(int r=(csr.y*8);r<(csr.y*8)+8;r++){
        clear_scanline_from_cursor(r);
    }
}
void clear_text_row_to_cursor(){
    for(int r=(csr.y*8);r<(csr.y*8)+8;r++){
    clear_scanline_to_cursor(r);
    }
}
void clear_entire_screen(){
    for(int r=0;r<ROWS;r++){
        clear_entire_scanline(r);
    }
}

void clear_screen_from_csr(){
    clear_text_row_from_cursor();
    for(int r=((csr.y+1)*8);r<TEXTROWS*8;r++){
        clear_entire_scanline(r);
    }
}

void clear_screen_to_csr(){
    clear_text_row_to_cursor();
    for(int r=((csr.y+1)*8);r<TEXTROWS*8;r++){
        clear_entire_scanline(r);
    }

}





void shuffle(){
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

void print_cursor(){
    // these are the same

        if(cursor_visible==false) return;

        int scanlineNumber;
        int characterPosition;
        int cursor_buffer_counter = 0;
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (csr.y*8)+r;
            characterPosition = (csr.x*8);
            for(int bit=0;bit<8;bit++){
                uint16_t pixel = ptr[scanlineNumber]->pixels[characterPosition+bit];
                uint16_t newPixel = pixel==background_colour ? foreground_colour : background_colour;
                cursor_buffer[cursor_buffer_counter++]=pixel;
                ptr[scanlineNumber]->pixels[characterPosition+bit] = newPixel;
            }
        }
}
void clear_cursor(){

        if(cursor_visible==false) return;

        // put back what's in the cursor_buffer

        int scanlineNumber;
        int characterPosition;
        int cursor_buffer_counter = 0;
        for(int r=0;r<8;r++){   // r is a row offset
            scanlineNumber = (csr.y*8)+r;
            characterPosition = (csr.x*8);
            for(int bit=0;bit<8;bit++){
                uint16_t newPixel = cursor_buffer[cursor_buffer_counter++];
                ptr[scanlineNumber]->pixels[characterPosition+bit] = newPixel;

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
        if(esc_parameters[0]==0){
            rvs = false;
        }
        if(esc_parameters[0]==7){
            rvs = true;
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
            clear_text_row_from_cursor();
        break;
            case 1:
            // clear from cursor to beginning of the line
            clear_text_row_to_cursor();
        break;
            case 2:
            // clear entire line
            clear_entire_text_row(csr.y);
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
}

void print_row_of_logo(char str[], int x, int scanlineNumber){
    for(int i=0;i<45;i++){
        if(str[i]=='X'){
            print_logo_element(x+(5*i),scanlineNumber);
        }
        else{
            // do nothing
        }
    }
}

void print_logo(){
    print_row_of_logo("XXXXXX   XXXXX   XXXXX   XXXXX    XX   XX XX ",7,1);
    print_row_of_logo("XX   XX XX   XX XX   XX XX  XXX  XXX   XX XX ",6,6);
    print_row_of_logo("XX   XX XX          XX  XX X XX   XX  XX  XX ",5,11);
    print_row_of_logo("XXXXXX  XX       XXXX   XX X XX   XX  XX  XX ",4,16);
    print_row_of_logo("XX   XX XX      XX      XX X XX   XX  XXXXXXX",3,21);
    print_row_of_logo("XX   XX XX   XX XX   XX XXX  XX   XX      XX ",2,26);
    print_row_of_logo("XX   XX  XXXXX  XXXXXXX  XXXXX  XXXXXX    XX",1,31);
}





void prepare_text_buffer(){
    // do we need to blank them, ie fill with 0?
    //foreground_colour = palette[9];


    foreground_colour = PICO_SCANVIDEO_PIXEL_FROM_RGB8(68,77,142);
    background_colour = palette[15];

    background_colour = PICO_SCANVIDEO_PIXEL_FROM_RGB8(68,77,142);
    foreground_colour = palette[15];


    reset_escape_sequence();

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

		// print cursor
    make_cursor_visible(true);
		clear_cursor(); // make as 80 columns²
    print_cursor();  // turns on
}

void display_terminal(){
		// do we need to blank them, ie fill with 0?
    char msg[80];

    clear_entire_screen();
    print_logo();

    csr.y=9;
		print_string("Menu : CTRL+SHIF+M\r\n");
		sprintf(msg, "\r\nTinyUSB=%d.%d.%d, ", TUSB_VERSION_MAJOR, TUSB_VERSION_MINOR,TUSB_VERSION_REVISION);
		print_string(msg);
		sprintf(msg, "Keymap=%s rev %d\r\n", KEYMAP, KEYMAP_REV );
		print_string(msg);
		// Update "project(picoterm VERSION 1.0)" in CMakeList
		sprintf(msg, "PicoTerm Colour %s  S. Dixon\r\n", CMAKE_PROJECT_VERSION );
		print_string(msg);


}

void display_menu(){
    reset_escape_sequence();
    clear_entire_screen();
    csr.x = 0; csr.y = 0;

    print_string("       >>>>  PicoTerm Menu <<<<\r\n");
		print_string("\r\n");
		print_string("+- Terminal test   (reboot) -+\r\n");
		print_string("|   0 option    3 option     |\r\n" );
		print_string("|   1 option    4 option     |\r\n" );
		print_string("|   2 option    5 option     |\r\n" );
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


void handle_udchar_data(uint8_t d){

    data_bytes_expected--;
    // doing this first makes it 7 - 0
    static int bytenum;
    bytenum = 7-data_bytes_expected;
    //to make 0-7
    bytenum+=((current_udchar-128)*8);

    custom_bitmap[bytenum] = d;

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
		config.colour_preference = _color;
    //write_data_to_flash();
		write_config_to_flash( &config );
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

                slip_character(asc-32,csr.x,csr.y);
                csr.x++;


                // this for disabling / enabling wrapping in terminal
                #ifndef LINEWRAP
                    constrain_cursor_values();
                #else
                    // this code for enabling wrapping in terminal
                    if(csr.x>=COLUMNS){
                        csr.x=0;
                        if(csr.y==TEXTROWS-1){
                            shuffle();
                        }
                        else{
                            csr.y++;
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
                    if(csr.x>0){
                        csr.x--;
                    }
                    break;
                    case LF:

                        if(csr.y==TEXTROWS-1){   // visiblerows is the count, csr is zero based
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
        }

    } // not esc sequence



}
