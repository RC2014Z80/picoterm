/* ==========================================================================
    Manage the adcanced interaction with the console (printing string,
		request input).

		Relies on the Picoterm_cooio.c to interact with screen
   ========================================================================== */

#ifndef _PICOTERM_STDIO_H_
#define _PICOTERM_STDIO_H_

void print_string(char str[] );
void print_char( char c );
char get_key( bool ascii ); // BLOCKING read_key with option to ascii only char
void get_string(char *str, int max_size);

#endif // _PICOTERM_STDIO_H_
