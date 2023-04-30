/* ==========================================================================
    CLI - Command Line Interface to decode command encoded via the Picoterm
		      "Command Line console"

   ========================================================================== */

#ifndef _CLI_H
#define _CLI_H

#include <stdbool.h>

#define NUMBER_OF_STRING 10 // number of tokens in the decomposed command
#define MAX_STRING_SIZE 25  // max size of a single token

void cli_init();
bool has_flag( char *flag_str,  char tokens[][MAX_STRING_SIZE] );
void cli_execute( char *str, int max_size );

#endif
