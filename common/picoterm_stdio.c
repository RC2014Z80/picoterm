/* ==========================================================================
    Manage the adcanced interaction with the console (printing string,
		request input).

		Relies on the Picoterm_cooio.c to interact with screen
   ========================================================================== */

#include "picoterm_core.h" // handle_new_character
#include "picoterm_conio.h" // readkey

#include "tusb.h"

//#include "../common/picoterm_debug.h"

/* picoterm_conio.c */
extern picoterm_conio_config_t conio_config;


void print_string(char str[] ){
	// Would it be more appropruate to use the put_char() and move the cursor?
	for(int i=0;i<strlen(str);i++)
		handle_new_character( str[i] );
}

void get_string(char *str, int max_size){
	// key-in a null terminated string (so max_size-1) until Return Key
	int pos = 0;
	for( int i = 0; i<max_size; i++)
		str[i] = 0;

	char ch = 0;
	while( ch != 13 ){
		tuh_task(); // allow keyboard input to get into the input buffer
		csr_blinking_task();
		ch = read_key(); // get last key-pressed from the buffer
		//debug_print("read_key done");
		if( (ch != 0) && (ch >= 32)){
			// sprintf(debug_msg,"%c", ch);
			// debug_print(debug_msg);
			cursor_visible( false );
			put_char( ch-32, conio_config.cursor.pos.x, conio_config.cursor.pos.y ); // ascii -32
			str[pos] = ch;
			if( pos < (max_size-1) )
				pos++;
				conio_config.cursor.pos.x++;
			cursor_visible( true );
		}
	}
	cursor_visible( false );
}
