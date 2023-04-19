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


#include "picoterm_screen.h"
#include "tusb_option.h"
#include "../common/picoterm_config.h"
#include "../common/pmhid.h"
#include "main.h"
#include "picoterm_core.h"
#include "picoterm_conio.h"
#include "hardware/watchdog.h"
#include "../common/picoterm_stddef.h"

#define LINEWRAP        // comment out to disable line wrapping



/* #define CSRCHAR     128 */

/* #define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BSP         0x08
#define LF          0x0a
#define CR          0x0d
#define FF          0x0c */

/* Picoterm_i2c.c */
extern bool i2c_bus_available; // gp26 & gp27 are used as I2C (otherwise as simple GPIO)

// picoterm_conio.c
extern struct scanline *ptr[ROWS];

// common/picoterm_config.c
extern picoterm_config_t config;

/* ----------------------------------------------------------------------------
   - Toolbox
   ---------------------------------------------------------------------------*/

char handle_default_input(){
  // Make your own specialized menu input handler (if needed, see handle_menu_input)
  // and call it as needed from main.c::main()
  char _ch = read_key();
  return _ch;
}

/* --- CONFIG ----------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/

void display_config(){
    char msg[60];
    reset_escape_sequence();
    clear_primary_screen();
		move_cursor_home();

    print_string("       >>>>  PicoTerm Menu <<<<\r\n");
		print_string("\r\n");
		print_string("+- Term. color (80col only) ---+\r\n");
		print_string("| 0 reserved  3 reserved       |\r\n" );
		print_string("| 1 reserved  4 reserved       |\r\n" );
		print_string("| 2 reserved  5 reserved       |\r\n" );
		print_string("+- Serial  Baud ---------------+\r\n" );
		sprintf(msg, "|%s115200 %s57600 %s19200   |\r\n", (config.baudrate==115200)?"<a>":" a " , (config.baudrate==57600)?"<b>":" b ", (config.baudrate==19200)?"<c>":" c " );
		print_string(msg);
		sprintf(msg, "|%s9600   %s4800  %s2400    |\r\n", (config.baudrate==9600)?"<d>":" d " , (config.baudrate==4800)?"<e>":" e ", (config.baudrate==2400)?"<f>":" f " );
		print_string(msg);
		sprintf(msg, "|%s1200   %s300              |\r\n", (config.baudrate==1200)?"<g>":" g " , (config.baudrate==300)?"<h>":" h " );
		print_string(msg);
		print_string("+- Data bits ------------------+\r\n" );
		sprintf(msg, "|%s 7 bits    %s 8 bits      |\r\n",  (config.databits==7)?"<7>":" 7 ", (config.databits==8)?"<8>":" 8 " );
		print_string(msg);
		print_string("+- Parity ---------------------+\r\n" );
		// "|   n None    o Odd    v Even  |  w: 1 bit   |\r\n"
		sprintf(msg, "|%sNone   %sOdd   %sEven    |\r\n", (config.parity==UART_PARITY_NONE)?"<n>":" n " , (config.parity==UART_PARITY_ODD)?"<o>":" o ", (config.parity==UART_PARITY_EVEN)?"<v>":" v "  );
		print_string(msg);
		print_string("+- Stop bits ------------------+\r\n" );
		sprintf(msg, "|%s 1 bits  %s 2 bits        |\r\n", (config.stopbits==1)?"<w>":" w ", (config.stopbits==2)?"<x>":" x "  );
		print_string(msg);
		print_string("+------------------------------+\r\n" );
		print_string("\r\n(S upcase=save / ESC=close) ? ");



    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}

char handle_config_input(){
  // check if user selected an option THEN execute the appropriate action
  // return the pressed key if any.
  char _ch = read_key();
  if( (_ch==0) || (_ch==ESC) )
    return _ch;

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

	// change the color (no effect on the 40 col terminal)
  if( (_ch >= '0') && (_ch <= '5') ) {
		uint8_t _color = _ch - 48; // 48->53 to 0->5 (WHITE->GREEN3)
		config.colour_preference = _color;
		// build_font(); not available in the 40 column version
	}

	// Baud rate
	if( ( _ch >= 'a') && (_ch <= 'h') ) {
		switch( _ch ){
			case 'a':
				config.baudrate = 115200;
				break;
			case 'b':
				config.baudrate = 57600;
				break;
			case 'c':
				config.baudrate = 19200;
				break;
			case 'd':
				config.baudrate = 9600;
				break;
			case 'e':
				config.baudrate = 4800;
				break;
			case 'f':
				config.baudrate = 2400;
				break;
			case 'g':
				config.baudrate = 1200;
				break;
			case 'h':
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

  return _ch;
}


/* --- TERMINAL ---------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/


void print_row_of_logo(char str[], int x, int scanlineNumber){
    for(int i=0;i<45;i++){
        if(str[i]=='X'){
            print_element(x+(5*i),scanlineNumber, custom_bitmap );
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


void display_terminal(){
    char msg[80];
    clear_primary_screen();
    print_logo();

    // csr.y=9;
		move_cursor_at( 9, 1 );
		print_string("\r\nS.Dixon & D.Meurisse  Menu:CTRL+SHIF+M\r\n");
		sprintf(msg, "\r\nTinyUSB=%d.%d.%d, ", TUSB_VERSION_MAJOR, TUSB_VERSION_MINOR,TUSB_VERSION_REVISION);
		print_string(msg);
		// Should only displays ASCII - Graphical font is not supported (ANSI) in 40 COLs
		sprintf(msg, "Keymap=%s rev %d, %s\r\n", KEYMAP, KEYMAP_REV,  config.font_id==FONT_ASCII ? "ASCII" : "ANSI" );
		print_string(msg);
		sprintf(msg, "Buzzer/USB-power on %s\r\n", i2c_bus_available==true ? "I2C" : "GPIO" );
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
		// Update "project(picoterm VERSION 1.0)" in CMakeList
		sprintf(msg, "PicoTerm %s @ %i bds %i%c%i\r\n", CMAKE_PROJECT_VERSION, config.baudrate, config.databits, _parity, config.stopbits );
		print_string(msg);
}
