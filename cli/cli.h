/* ==========================================================================
    CLI - Command Line Interface to decode command encoded via the Picoterm
		      "Command Line console"

   ========================================================================== */

#ifndef _CLI_H
#define _CLI_H

void cli_init();
void cli_execute( char *str, int max_size );

#endif
