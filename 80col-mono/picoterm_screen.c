/* ==========================================================================
        Manage the screen rendering :
              * display_x functions &
              * keyboard callback function for keyboard
   ========================================================================== */

//#include "picoterm_core.h"
#include "../common/pmhid.h" // keyboard definitions
#include "../common/picoterm_config.h"
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_harddef.h"
#include "../common/picoterm_stdio.h"
#include "picoterm_core.h" // handle_new_character
#include "tusb_option.h"
#include "picoterm_conio.h"
#include "main.h" // UART_ID
#include "hardware/watchdog.h"
#include <stdio.h>
#include "../cli/cli.h"
#include "../common/picoterm_debug.h"



/* Picoterm_i2c.c */
extern bool i2c_bus_available; // gp26 & gp27 are used as I2C (otherwise as simple GPIO)

/* picoterm_config.c */
extern picoterm_config_t config; // Issue #13, awesome contribution of Spock64

/* picoterm_conio.c */
extern picoterm_conio_config_t conio_config;

/* picoterm_logo.c */
extern const int LOGO_LINES;
extern const char * PICOTERM_LOGO[];

/* ----------------------------------------------------------------------------
   - Toolbox
   ---------------------------------------------------------------------------*/

char handle_default_input(){
  // Make your own specialized menu input handler (if needed, see handle_menu_input)
  // and call it as needed from main.c::main()
  char _ch = read_key();
  return _ch;
}

/* --- COMMAND Interpreter ----------------------------------------------------
   -
   ---------------------------------------------------------------------------*/

 void display_command(){
   clrscr();
   move_cursor_home();

	 print_string( "---- Picoterm command interpreter ----\r\nUse: exit, list, <command> ?, <command> -h\r\n" );
}


char handle_command_input(){
  // Ask user to enter command followed by RETURN key then execute it!
  //
	char _cmd[80];
	while (strcmp( _cmd, "exit") != 0) {
		print_string( "\r\n$ " );
		cursor_visible( true );
		get_string( _cmd, sizeof(_cmd) );
		cursor_visible( false );
		debug_print( "Invoke cli to execute:");
		debug_print( _cmd );
		print_string( "\r\n" );
		cli_execute( _cmd, sizeof(_cmd) );
	}

	// Inform callee to close the screen
	cursor_visible( true );
	return ESC;
}
/* --- CHARSET ----------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/

void display_charset(){
  char msg[80];
  char _c;
  // reset_escape_sequence(); LOOKS not usefull from screen!
  clrscr();
  move_cursor_home(); //csr.x = 0; csr.y = 0;

  print_nupet( "\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 Current Charset \x0A6\x0A6\r\n" , config.font_id ); // strip Nupetscii when not activated
  print_string( "     0 1 2 3 4 5 6 7 8 9 A B C D E F\r\n");
  print_nupet( "   \x0B0\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0AE\r\n"  , config.font_id );
  for( char line=2; line<16; line++ ){
    sprintf( msg, "%02X \x0C2 ", line*16 );
    print_nupet( msg , config.font_id ); // strip Nupetscii when not activated
    for( char index=0; index<=15; index++ ){
      _c = line*16+index;
      sprintf( msg, "%c ", _c );
      print_string( msg );
    }
    print_nupet("\x0C2\r\n", config.font_id );
    // Insert a index line in the middle for easier reading
    if( line==8 ){
      print_nupet( "   \x0AB\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3\r\n", config.font_id );
      print_nupet( "   \x0C2 0 1 2 3 4 5 6 7 8 9 A B C D E F \x0C2\r\n" , config.font_id );
      print_nupet( "   \x0AB\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3\r\n", config.font_id );
    }
  }
  print_nupet( "   \x0AD\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0BD\r\n", config.font_id );
  print_string( "     0 1 2 3 4 5 6 7 8 9 A B C D E F\r\n");

  print_string("\r\n(ESC=close) ? ");
  cursor_visible(true);
  clear_cursor();  // so we have the character
  print_cursor();  // turns on
}

/* --- CONFIG ----------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/

void display_config(){
    char msg[80];
    // reset_escape_sequence(); LOOKS not usefull from screen!
    clrscr();
    move_cursor_home();//csr.x = 0; csr.y = 0;

    print_nupet("\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 PicoTerm Menu \x0A6\x0A6\r\n" , config.font_id ); // strip graphical when not activated
    print_nupet("\x0E2\x0E1 Terminal Colors \x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E4\r\n", config.font_id );
    sprintf(msg, "\x0E0  %s0 White   %s1 Light Amber %s2 Dark Amber   \x0E0\r\n", (config.colour_preference==0)?"\x0D1":" ", (config.colour_preference==1)?"\x0D1":" ", (config.colour_preference==2)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    sprintf( msg, "\x0E0  %s3 Green1  %s4 Green2      %s5 Green3       \x0E0\r\n", (config.colour_preference==3)?"\x0D1":" ", (config.colour_preference==4)?"\x0D1":" ", (config.colour_preference==5)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    sprintf( msg, "\x0E0  %s6 Purple                                 \x0E0\r\n", (config.colour_preference==6)?"\x0D1":" " );
    print_nupet( msg, config.font_id );
    print_nupet("\x0E8\x0C3 Serial  Baud \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B2\x0C3 Data bit \x0C3\x0C3\x0E9\r\n", config.font_id );
    sprintf(msg, "\x0E0  %sa 115200 %sb 57600 %sc 38400 \x0C2 %s8  8 bits  \x0E0\r\n", (config.baudrate==115200)?"\x0D1":" " , (config.baudrate==57600)?"\x0D1":" ", (config.baudrate==38400)?"\x0D1":" ", (config.databits==8)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    sprintf(msg, "\x0E0  %sd 19200  %se 9600  %sf 4800  \x0C2 %s7  7 bits  \x0E0\r\n", (config.baudrate==19200)?"\x0D1":" " , (config.baudrate==9600)?"\x0D1":" ", (config.baudrate==4800)?"\x0D1":" ", (config.databits==7)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    sprintf(msg, "\x0E0  %sg 2400   %sh 1200  %si 300   \x0C2             \x0E0\r\n", (config.baudrate==2400)?"\x0D1":" " , (config.baudrate==1200)?"\x0D1":" " , (config.baudrate==300)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    print_nupet("\x0E8\x0C3 Parity \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0DB\x0C3 Stop bits \x0C3\x0E9\r\n", config.font_id );
    sprintf(msg, "\x0E0  %sn None   %so Odd   %sv Even  \x0C2 %sw  1 bit   \x0E0\r\n", (config.parity==UART_PARITY_NONE)?"\xD1":" " , (config.parity==UART_PARITY_ODD)?"\xD1":" ", (config.parity==UART_PARITY_EVEN)?"\xD1":" " , (config.stopbits==1)?"\xD1":" "  );
    print_nupet(msg, config.font_id );
    sprintf(msg, "\x0E8\x0C3 Charset \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B3 %sx  2 bits  \x0E0\r\n", (config.stopbits==2)?"\xD1":" " );
    print_nupet(msg, config.font_id );
    sprintf(msg, "\x0E0  %sl ASCII (7bits+reverse)    \x0C2             \x0E0\r\n", (config.font_id == FONT_ASCII)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    sprintf(msg, "\x0E0  %sm ANSI Graphic (8bits)     \x0C2             \x0E0\r\n", (config.font_id > FONT_ASCII)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    print_nupet("\x0E8\x0C3 ANSI Graphic font \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0B1\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0E9\r\n", config.font_id );
    sprintf(msg, "\x0E0  %sp NupetSCII Mono8    %sq CP437 Mono8      \x0E0\r\n", (config.graph_id==FONT_NUPETSCII_MONO8)?"\x0D1":" ", (config.graph_id==FONT_CP437_MONO8)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
		sprintf(msg, "\x0E0  %sr NupetSCII OlivettiT%ss CP437 OlivettiT  \x0E0\r\n", (config.graph_id==FONT_NUPETSCII_OLIVETTITHIN)?"\x0D1":" ", (config.graph_id==FONT_CP437_OLIVETTITHIN)?"\x0D1":" " );
    print_nupet(msg, config.font_id );
    print_nupet("\x0E5\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E1\x0E7\r\n", config.font_id );
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
    build_font( config.font_id ); // rebuilt font
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
  }
  // Parity configuration
  if ( ( _ch == 'n') || (_ch == 'o') || (_ch == 'v')) {
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
  }
  // ASCII / Graphical font configuration @ Startup
  if ( ( _ch >= 'l') && (_ch <= 'm') ) {
    switch( _ch ){
      case 'l':
        config.font_id = FONT_ASCII; // normal font
        break;
      case 'm':
        config.font_id = config.graph_id; //Graphical font (FONT_NUPETSCII_MONO8);
        break;
    }
    select_graphic_font( config.font_id );
    build_font(config.font_id);
    conio_config.cursor.symbol = get_cursor_char( config.font_id, CURSOR_TYPE_DEFAULT ) - 0x20;
  }
  // Select the Graphical font to be used
  if ( ( _ch >= 'p') && (_ch <= 's') ) {
    switch( _ch ){
      case 'p':
        config.graph_id = FONT_NUPETSCII_MONO8;
        break;
      case 'q':
        config.graph_id = FONT_CP437_MONO8;
        break;
      case 'r':
        config.graph_id = FONT_NUPETSCII_OLIVETTITHIN;
        break;
      case 's':
        config.graph_id = FONT_CP437_OLIVETTITHIN;
        break;
    }
    if( config.font_id != FONT_ASCII ) {
      config.font_id = config.graph_id; // set the Graphic font to font_id
      conio_config.ansi_font_id = config.font_id; // Terminal should be aware of the selected graphical font
      select_graphic_font( config.font_id );
      conio_config.cursor.symbol = get_cursor_char( config.font_id, CURSOR_TYPE_DEFAULT ) - 0x20;
      build_font(config.font_id);
    }
  }

	// redisplay the configuration in any situations
	display_config();
  return _ch;
}


/* --- HELP -------------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/

void display_help(){
  char msg[80];
  char _c;
  // reset_escape_sequence(); LOOKS not usefull from screen!
  clrscr();
  move_cursor_home(); // csr.x = 0; csr.y = 0;
  print_nupet("\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6\x0A6 PicoTerm Help \x0A6\x0A6\r\n", config.font_id );
  print_nupet("\x0B0\x0C3 Keyboard Shortcut \x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0AE\r\n", config.font_id );
	print_nupet("\x0C2 \x083 Shift+Ctrl+C : Command Line Interface        \x0C2\r\n", config.font_id ); // strip Nupetscii when not activated
  print_nupet("\x0C2 \x083 Shift+Ctrl+H : Help screen                   \x0C2\r\n", config.font_id ); // strip Nupetscii when not activated
  print_nupet("\x0C2 \x083 Shift+Ctrl+L : Toggle ASCII/ANSI charset     \x0C2\r\n", config.font_id );
  print_nupet("\x0C2 \x083 Shift+Ctrl+M : Configuration menu            \x0C2\r\n", config.font_id );
  print_nupet("\x0C2 \x083 Shift+Ctrl+N : Display current charset       \x0C2\r\n", config.font_id );
  print_nupet("\x0C2                                                \x0C2\r\n", config.font_id );
  print_nupet("\x0AD\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0C3\x0BD\r\n", config.font_id );

  print_string("\r\n(ESC=close) ? ");
  cursor_visible(true);
  clear_cursor();  // so we have the character
  print_cursor();  // turns on
}


/* --- TERMINAL ---------------------------------------------------------------
   -
   ---------------------------------------------------------------------------*/
#define n_array (sizeof (logo) / sizeof (const char *))

void display_terminal(){
    char msg[80];

    clrscr();
    move_cursor_home(); // csr.x = 0; csr.y = 0;

    for( int i=0; i < LOGO_LINES; i++ ){
      print_string( (char *)PICOTERM_LOGO[i] );
    }
    sprintf(msg, "TinyUSB=%d.%d.%d, ", TUSB_VERSION_MAJOR, TUSB_VERSION_MINOR,TUSB_VERSION_REVISION);
    print_string(msg);
    sprintf(msg, "Keymap=%s rev %d, ", KEYMAP, KEYMAP_REV );
    print_string(msg);
    sprintf(msg, "%s (%s)\r\n", config.font_id==FONT_ASCII ? "ASCII" : "ANSI", get_font_name(config.graph_id) ); // ANSI graphical font name in parenthesis
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
    // Update "project(picoterm VERSION 1.1)" in CMakeList
    sprintf(msg, "PicoTerm %s @ %i bds %i%c%i\r\n", CMAKE_PROJECT_VERSION, config.baudrate, config.databits, _parity, config.stopbits );
    print_string(msg);


    // print cursor
    cursor_visible(true);
    clear_cursor();  // so we have the character
    print_cursor();  // turns on
}
