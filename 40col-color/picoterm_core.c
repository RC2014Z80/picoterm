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
#include "../common/picoterm_debug.h"
#include "../common/picoterm_config.h"
#include "../common/picoterm_conio_config.h"
#include "../common/picoterm_dec.h"
#include "../common/picoterm_cursor.h"

#include "main.h" // UART_ID

/* picoterm_conio.c */
extern picoterm_conio_config_t conio_config;
extern picoterm_config_t config;
extern uint16_t foreground_colour;
extern uint16_t background_colour;

// 0 = BUZZER_STOPPED, 1 = BUZZER_SIGNAL (Play Please), 2 = BUZZER_RUNNING
char bell_state = 0;
bool insert_mode = false;

char get_bell_state() { return bell_state; }
void set_bell_state(char state) { bell_state = state; }

// early declaration
void reset_escape_sequence();
// Command answer
void response_VT100OK();
void response_VT100ID();
void response_csr();

// escape sequence state
#define ESC_READY               0
#define ESC_ESC_RECEIVED        1
#define ESC_PARAMETER_READY     2

#define MAX_ESC_PARAMS          5


static int esc_state = ESC_READY;
static int esc_parameters[MAX_ESC_PARAMS];
static bool parameter_q; // true when ? in request
static bool parameter_p; // true when parenthesis in request
static bool parameter_sp; // true when ESC sequence contains a space
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
    insert_mode = false;

    conio_reset();
    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
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
    // VT100 Support
    switch(esc_final_byte){
        case 'H':
        case 'f':
            // Moves the cursor to row n, column m
            //Move cursor to upper left corner ESC [H
            //Move cursor to upper left corner ESC [;H
            //Move cursor to screen location v,h ESC [<v>;<h>H
            //Move cursor to upper left corner ESC [f
            //Move cursor to upper left corner ESC [;f
            //Move cursor to screen location v,h ESC [<v>;<h>f

            n = esc_parameters[0];
            m = esc_parameters[1];
            if(n == 0) n = 1;
            if(m == 0) m = 1;
            move_cursor_at( n, m );
            break;

        case 'E': // ESC[#E moves cursor at beginning of next line, # lines down
            n = esc_parameters[0];
            if(n==0)n=1;
            // these are ONE based
            move_cursor_at( conio_config.cursor.pos.y+n+1, 1); // y, x
            break;

        case 'F':  // ESC[#F  moves cursor to beginning of previous line, # lines up
            n = esc_parameters[0];
            if(n==0)n=1;
            // these are ONE based
            move_cursor_at( conio_config.cursor.pos.y - n+1, 1);
            break;

        case 'd': // ESC[#d  moves cursor to an absolute # line
            n = esc_parameters[0];
            n--;
            // these are ONE based
            move_cursor_at( n+1, conio_config.cursor.pos.x+1 );
            break;

        case 'G': // ESC[#G  moves cursor to column #
            n = esc_parameters[0];
            n--;
            // there are ONE based
            move_cursor_at( conio_config.cursor.pos.y+1, n+1 );
            break;

        case 'h':
            //[ 2 h    Keyboard locked
            //[ 4 h    Insert mode selected
            //[ 20 h    Set new line mode

            //[ ? 1 h       Set cursor key to application
            //[ ? 2 h       Set ANSI (versus VT52)
            //[ ? 3 h    132 Characters on
            //[ ? 4 h    Smooth Scroll on
            //[ ? 5 h    Inverse video on
            //[ ? 7 h    Wraparound ON
            //[ ? 8 h    Autorepeat ON
            //[ ? 9 h       Set 24 lines per screen (default)
            //[ ? 12 h    Text Cursor Enable Blinking
            //[ ? 14 h    Immediate operation of ENTER key
            //[ ? 16 h    Edit selection immediate
            //[ ? 25 h    Cursor ON
            //[ ? 47 h      save screen
            //[ ? 50 h    Cursor ON
            //[ ? 75 h    Screen display ON
            //[ ? 1049 h  enables the alternative buffer
            if(parameter_q){
                if(esc_parameters[0]==25 || esc_parameters[0]==50){
                    // show csr
                    cursor_visible(true);
                }
                else if(esc_parameters[0]==7){
                    //Auto-wrap mode on (default) ESC [?7h
                    conio_config.wrap_text = true;
                }
                else if(esc_parameters[0]==9){
                    //Set 24 lines per screen (default)
                    terminal_reset(); // reset to simulate change
                }
                else if(esc_parameters[0]==12){
                    //Text Cursor Enable Blinking
                    conio_config.cursor.state.blinking_mode = true;
                }
                else if(esc_parameters[0]==47 || esc_parameters[0]==1047){
                    //TODO: save screen
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?47h or [?1047h  save screen" );
                    //copy_main_to_secondary_screen();
                }
                else if(esc_parameters[0]==1048){
                    //save cursor
                    save_cursor_position();
                }
                else if(esc_parameters[0]==1049){
                    //TODO: save cursor and save screen
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?1049h  enables the alternative buffer" );
                    //save_cursor_position();
                    //copy_main_to_secondary_screen();
                }
            }
            else{
                if(esc_parameters[0]==4){
                    //Insert mode selected
                    insert_mode = true;
                }
            }
            break; // case 'h'


        case 'l':
            //[ 2 l    Keyboard unlocked
            //[ 4 l    Replacement mode selected
            //[ 20 l    Set line feed mode

            //[ ? 1 l       Set cursor key to cursor
            //[ ? 2 l       Set VT52 (versus ANSI)
            //[ ? 3 l    80 Characters on
            //[ ? 4 l    Jump Scroll on
            //[ ? 5 l    Normal video off
            //[ ? 7 l    Wraparound OFF
            //[ ? 8 l    Autorepeat OFF
            //[ ? 9 l       Set 36 lines per screen
            //[ ? 12 l      Text Cursor Disable Blinking
            //[ ? 14 l      Deferred operation of ENTER key
            //[ ? 16 l      Edit selection deferred
            //[ ? 25 l      Cursor OFF
            //[ ? 47 l    restore screen
            //[ ? 50 l      Cursor OFF
            //[ ? 75 l      Screen display OFF

            //[ ? 1049 l  disables the alternative buffer

            if(parameter_q){
                if(esc_parameters[0]==25 || esc_parameters[0]==50){
                    // hide csr
                    cursor_visible(false);
                }
                else if(esc_parameters[0]==2){
                    //TODO: Set VT52 (versus ANSI)
                    //mode = VT52;
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?2l set VT52 (versus ANSI)" );
                }
                else if(esc_parameters[0]==7){
                    //Auto-wrap mode off ESC [?7l
                    conio_config.wrap_text = false;
                }
                else if(esc_parameters[0]==9){
                    //Set 36 lines per screen
                    terminal_reset(); // reset to simulate change
                }
                else if(esc_parameters[0]==12){
                    //Text Cursor Disable Blinking
                    conio_config.cursor.state.blinking_mode = false;
                }
                else if(esc_parameters[0]==47 || esc_parameters[0]==1047){
                    //TODO: restore screen
                    //copy_secondary_to_main_screen();
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?47l or [?1047l  restore screen" );
                }
                else if(esc_parameters[0]==1048){
                    //TODO: restore cursor
                    //copy_secondary_to_main_screen();
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?1048l  restore screen" );
                }
                else if(esc_parameters[0]==1049){
                    //TODO: restore screen and restore cursor
                    //copy_secondary_to_main_screen();
                    //restore_cursor_position();
                    debug_print( "picoterm_core: esc_sequence_received() to be implemented" );
                    debug_print( "[?1049l  restore screen & Cursor" );
                }
            }
            else{
                if(esc_parameters[0]==4){
                    //Replacement mode selected
                    insert_mode = false;
                }
            }
            break; // case 'l'


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
                background_colour = palette[esc_parameters[0]-40];
            }
            if(esc_parameters[0]>=90 && esc_parameters[0]<=97){
                foreground_colour = palette[esc_parameters[0]-82]; // 90 is palette[8]
            }
            if(esc_parameters[0]>=100 && esc_parameters[0]<=107){
                background_colour = palette[esc_parameters[0]-92];  // 100 is palette[8]
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
            break; // case 'm'


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
            break; // case 'J'


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
            break; // case 'K'


        case 'A': // Cursor Up - Moves the cursor n (default 1) cells
            n = esc_parameters[0];
            move_cursor_up(n);
            break;

        case 'B': // Cursor Down - Moves the cursor n (default 1) cells
            n = esc_parameters[0];
            move_cursor_down(n);
            break;

        case 'C': // Cursor Forward - Moves the cursor n (default 1) cells
            n = esc_parameters[0];
            move_cursor_forward(n);
            break;

        case 'D': // Cursor Backward - Moves the cursor n (default 1) cells
            n = esc_parameters[0];
            move_cursor_backward(n);
            break;

        case 'S': // Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
            n = esc_parameters[0];
            if(n==0) n=1;
            for(int i=0;i<n;i++)
              shuffle_down();
            break;

        case 'T': // Scroll whole page down by n (default 1) lines. New lines are added at the top. (not ANSI.SYS)
            n = esc_parameters[0];
            if(n==0) n=1;
            for(int i=0;i<n;i++)
              shuffle_up();
            break;

        // MORE

        case 'L': // 'INSERT LINE' - scroll rows down from and including cursor position. (blank the cursor's row??)
            n = esc_parameters[0];
            if(n==0)n=1;
            insert_lines(n);
            break;

        case 'M': // 'DELETE LINE' - delete row at cursor position, scrolling everything below, up to fill. Leaving blank line at bottom.
            n = esc_parameters[0];
            if(n==0)n=1;
            delete_lines(n);
            break;

        case 'P': // 'DELETE CHARS' - delete <n> characters at the current cursor position, shifting in space characters from the right edge of the screen.
            n = esc_parameters[0];
            if(n==0)n=1;
            delete_chars(n);
            break;

        case 'X': // 'ERASE CHARS' - erase <n> characters from the current cursor position by overwriting them with a space character.
            n = esc_parameters[0];
            if(n==0)n=1;
            erase_chars(n);
            break;

        case '@': // 'Insert Character' - insert <n> spaces at the current cursor position, shifting all existing text to the right. Text exiting the screen to the right is removed.
            n = esc_parameters[0];
            if(n==0)n=1;
            insert_chars(n);
            break;

        case 'q':
            if(parameter_sp){
                parameter_sp = false;
                //ESC [ 0 SP q  User Shape  Default cursor shape configured by the user
                //ESC [ 1 SP q  Blinking Block  Blinking block cursor shape
                //ESC [ 2 SP q  Steady Block  Steady block cursor shape
                //ESC [ 3 SP q  Blinking Underline  Blinking underline cursor shape
                //ESC [ 4 SP q  Steady Underline  Steady underline cursor shape
                //ESC [ 5 SP q  Blinking Bar  Blinking bar cursor shape
                //ESC [ 6 SP q  Steady Bar  Steady bar cursor shape
                conio_config.cursor.symbol = get_cursor_char( FONT_ASCII /*config.font_id*/ , esc_parameters[0] ) - 0x20; // parameter correspond to picoterm_cursor.h::CURSOR_TYPE_xxx
                conio_config.cursor.state.blinking_mode = get_cursor_blinking( config.font_id, esc_parameters[0] );
            }
            break; // case q

        case 'c':
            response_VT100ID();
            break;

        case 'n':
            if (esc_parameters[0]==5){
                response_VT100OK();
            }
            else if (esc_parameters[0]==6){
                response_csr();
            }
            break; // case 'n'

        // Does U is part of VT100 ?
        case 'U':
            if(parameter_q){
                // user-defined character data follows
                // first parameter is the character number, which must be 128-255 inclusive
                // follow with 8 bytes which will be inserted into the UD character space

                data_purpose=UDCHAR;
                current_udchar = esc_parameters[0];
                data_bytes_expected = 8;
            }
            break; // case 'U'

    } // eof switch(esc_final_byte)
  } // eof if(esc_c1=='[')
  else{
    // ignore everything else
  }
  // End OF VT100

  // Both VT52 & VT100
  if(esc_c1=='('){
      // CSI
      switch(esc_final_byte){
      case 'B':
          // display ascii chars (not letter) but stays in NupetScii font
          // to allow swtich back to DEC "single/double line" drawing.
          conio_config.dec_mode = DEC_MODE_NONE;
          break;
      case '0':
          conio_config.dec_mode = DEC_MODE_SINGLE_LINE;
          break;
      case '2':
          conio_config.dec_mode = DEC_MODE_DOUBLE_LINE;
          break;
      default:
          conio_config.dec_mode = DEC_MODE_NONE;
          break;
      }
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
  // Ask Terminal core to handle this new character
  if(esc_state != ESC_READY){
      // === ESC SEQUENCE ====================================================
      switch(esc_state){
          case ESC_ESC_RECEIVED:
              // --- waiting on c1 character ---
              // c1 is the first parameter after the ESC
              if( (asc=='(') || (asc=='[') ){
                  // 0x9B = CSI, that's the only one we're interested in atm
                  // the others are 'Fe Escape sequences'
                  // usually two bytes, ie we have them already.
                  if(asc=='['){    // ESC+[ =  0x9B){
                      esc_c1 = asc;
                      esc_state=ESC_PARAMETER_READY; // Lets wait for parameter
                      clear_escape_parameters();
                      // number of expected parameter depends on next caracter.
                  }
                  else if(asc=='('){    // ESC+(
                      esc_c1 = asc;
                      esc_state=ESC_PARAMETER_READY; // Lets wait for parameter
                      clear_escape_parameters();
                      parameter_p=true; // we just expecty a single parameter.
                  }
                  // other type Fe sequences go here
                  else
                      // for now, do nothing
                      reset_escape_sequence();
              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT100 / VT52 ----------------------------------------------
              else if(asc=='c'){ // mode==BOTH VT52 / VT100 Commands
                    terminal_reset();
                    reset_escape_sequence();
              }
              else if (asc=='F' ){
                    debug_print( "picoterm_code: handle_new_character() - \\ESCF" );
                    // config.font_id=config.graph_id; // Enter graphic charset
                    // build_font( config.font_id );
                    conio_config.dec_mode = DEC_MODE_NONE; // use approriate ESC to enter DEC Line Drawing mode
                    reset_escape_sequence();
              }
              else if (asc=='G'){
                    debug_print( "picoterm_code: handle_new_character() - \\ESCG" );
                    //config.font_id=FONT_ASCII; // Enter ASCII charset
                    //build_font( config.font_id );
                    conio_config.dec_mode = DEC_MODE_NONE;
                    reset_escape_sequence();
              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT100 -----------------------------------------------------
              else if( true ){ //SUPPORT ONLY VT100:   if(mode==VT100){  // VT100 Commands

                if (asc=='7' ){
                    // save cursor position
                    save_cursor_position();
                    reset_escape_sequence();
                }
                else if (asc=='8' ){
                    // move to saved cursor position
                    restore_cursor_position();
                    reset_escape_sequence();
                }
                else if (asc=='D' ){
                    //cmd_lf();
                    move_cursor_lf( false ); // normal move
                    reset_escape_sequence();
                }
                else if (asc=='M' ){
                    //cmd_rev_lf();
                    move_cursor_lf( true ); // reverse move
                    reset_escape_sequence();
                }
                else if (asc=='E' ){
                    //cmd_lf();
                    move_cursor_lf(true); // normal move
                    reset_escape_sequence();
                }

              }
              // --- SINGLE CHAR escape ----------------------------------------
              // --- VT52 ------------------------------------------------------
              else if(false){  //SUPPORT ONLY VT100:   if(mode==VT52){ // VT52 Commands

                if (asc=='A' ){
                    move_cursor_up( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='B' ){
                    move_cursor_down( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='C' ){
                    move_cursor_forward( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='D' ){
                    move_cursor_backward( 1 );
                    reset_escape_sequence();
                }
                else if (asc=='H' ){
                    move_cursor_home();
                    reset_escape_sequence();
                }
                else if (asc=='I' ){
                    move_cursor_lf( true ); // reverse move
                    reset_escape_sequence();
                }
                else if (asc=='J' ){
                    clear_screen_from_cursor();
                    reset_escape_sequence();
                }
                else if (asc=='K' ){
                    clear_line_from_cursor();
                    reset_escape_sequence();
                }
                else if (asc=='Z' ){
                    //SUPPORT ONLY VT100:   response_VT52Z();
                    reset_escape_sequence();
                }
                else if (asc=='<' ){
                    //SUPPORT ONLY VT100:   mode = VT100;
                    reset_escape_sequence();
                }
                else
                    // unrecognised character after escape.
                    reset_escape_sequence();
            }
            // ==============

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
                    esc_sequence_received(); // execute esc sequence
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
                  if(esc_parameter_count<MAX_ESC_PARAMS) esc_parameter_count++;
              }
              else if(asc=='?'){
                  parameter_q=true;
              }
              else if(asc==' '){
                  parameter_sp=true;
              }
              else if(asc>=0x40 && asc<0x7E){
                  // final byte. Log and handle
                  esc_final_byte = asc;
                  esc_sequence_received(); // execute esc sequence
              }
              else{
                  // unexpected value, undefined
              }
              break;
      }

  }
  else

  // === data? ============================================================
  if(data_bytes_expected>0){
        if(data_purpose==ARBITRARY){

        }
        if(data_purpose==BITMAPDATA){

        }
        if(data_purpose==UDCHAR){
            handle_udchar_data((uint8_t)asc);
        }
  }
  else


  {
      // === regular characters ==============================================
      if(asc>=0x20 && asc<=0xFF){

          //if insert mode shift chars to the right
          if(insert_mode) insert_chars(1);

          // --- Strict ASCII <0x7f or Extended NuPetSCII <= 0xFF ---
          put_char(asc-32,conio_config.cursor.pos.x,conio_config.cursor.pos.y);
          conio_config.cursor.pos.x++;

          if(!conio_config.wrap_text){
            // this for disabling wrapping in terminal
            constrain_cursor_values();
          }
          else{
            // alternatively, use this code for enabling wrapping in terminal
            wrap_constrain_cursor_values();
          }
      }
      else if(asc==0x1B){
          // --- Begin of ESCAPE SEQUENCE ---
          esc_state=ESC_ESC_RECEIVED;
      }
      else {
          // --- return, backspace etc ---
          switch (asc){
              case BEL:
              bell_state = 1;
              break;

              case BSP:
                if( conio_config.cursor.pos.x>0 ){
                  conio_config.cursor.pos.x--;
                }
                break;

              case LF:
                //cmd_lf();
                move_cursor_lf( false ); // normal move
                break;

              case CR:
                conio_config.cursor.pos.x=0;
                break;

              case FF:
                clrscr();
                conio_config.cursor.pos.x=0; conio_config.cursor.pos.y=0;
                break;
          } // switch(asc)
      } // else
  } // eof Regular character

  if(conio_config.cursor.state.blink_state)
    conio_config.cursor.state.blink_state = false;
}


/*
void handle_new_character(unsigned char asc){
    // handle escape sequences
    if(esc_state != ESC_READY){
        // === ESC SEQUENCE ====================================================
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
                else if(asc==' '){
                  parameter_sp=true;
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
  } // esc_state != ESC_READY
  else
  {
      // === data? ============================================================
      if(data_bytes_expected>0){
            if(data_purpose==ARBITRARY){

            }
            if(data_purpose==BITMAPDATA){

            }
            if(data_purpose==UDCHAR){
                handle_udchar_data((uint8_t)asc);
            }
      }
      else {
        // === regular characters ==============================================
        if(asc>=0x20 && asc<=0xff){
                // If insert mode shift chars to the right
                if(insert_mode) insert_chars(1);

                // --- Strict ASCII <0x7f or Extended NuPetSCII <= 0xFF ---
                slip_character(asc-32,conio_config.cursor.pos.x,conio_config.cursor.pos.y);
                conio_config.cursor.pos.x++;



                if(!conio_config.wrap_text){
                    // this for disabling wrapping in terminal
                    constrain_cursor_values();
                }
                else {
                    // this code for enabling wrapping in terminal
                    // *** SHOULD BE REPLACED WITH wrap_constrain_cursor_values()
                    // *** AS DONE in as 80col version
                    // TODO:
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
                } // if( !conio_config.wrap_text )
        } else
          if(asc==0x1B) {
              // --- Begin of ESCAPE SEQUENCE ---
              esc_state=ESC_ESC_RECEIVED;
          } else
          {
            // --- return, backspace etc ---
            switch (asc){
                case BEL:
                    bell_state = 1; //BUZZER_SIGNAL
                    break;

                case BSP:
                    if(conio_config.cursor.pos.x>0)
                        conio_config.cursor.pos.x--;
                    break;

                case LF:
                    //if(conio_config.cursor.pos.y==TEXTROWS-1)   // visiblerows is the count, csr is zero based
                    //    shuffle_down();
                    //else
                    //  conio_config.cursor.pos.y++;
                    move_cursor_lf( false ); // normal move
                    break;

                case CR:
                    conio_config.cursor.pos.x=0;
                    break;

                case FF:
                    clear_primary_screen();
                    move_cursor_home();
                    //conio_config.cursor.pos.x=0; conio_config.cursor.pos.y=0;
                    break;
            } // switch(asc)
          } // else
      } // else
  } // not esc sequence
}
*/


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


void __send_string(char str[]){
  /* send string back to host via UART */
  char c;
  for(int i=0;i<strlen(str);i++){
      c = str[i];
      //insert_key_into_buffer( c );
      uart_putc (UART_ID, c);
  }
}

void response_VT100OK() {
    __send_string("\033[0n");
}

void response_VT100ID() {
    __send_string("\033[?1;0c");    // vt100 with no options
}

void response_csr() { // cursor position
    char s[20];
    sprintf(s, "\033[%d;%dR", conio_config.cursor.pos.y+1, conio_config.cursor.pos.x+1);
    __send_string(s);
}
