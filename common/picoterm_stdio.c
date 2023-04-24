/* ==========================================================================
    Manage the adcanced interaction with the console (printing string,
		request input).

		Relies on the Picoterm_cooio.c to interact with screen
   ========================================================================== */

#include "picoterm_core.h" // handle_new_character

void print_string(char str[] ){
	// Would it be more appropruate to use the put_char() and move the cursor?
	for(int i=0;i<strlen(str);i++)
		handle_new_character( str[i] );
}
