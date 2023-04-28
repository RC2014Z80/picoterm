/* ==========================================================================
    CLI - Command Line Interface to decode command encoded via the Picoterm
		      "Command Line console"

   ========================================================================== */

#include "user_funcs.h"
#include "tinyexpr.h"
#include <stdio.h>
#include <string.h>
#include "../common/picoterm_stdio.h"
#include "../common/picoterm_debug.h"

#define NUMBER_OF_STRING 10 // number of tokens in the decomposed command
#define MAX_STRING_SIZE 25

unsigned long int word_count(const char *string);
int parse_string(char *str, char tokens[][MAX_STRING_SIZE], char *delim);

// usr_funcs user_functions[MAX_USER_FUNCTIONS];

/* user_funcs.c */
extern usr_funcs user_functions[MAX_USER_FUNCTIONS];

int token_cnt = 0;
char tokens[NUMBER_OF_STRING][MAX_STRING_SIZE];
//char buffer[BUF_SIZE], prev_buffer[BUF_SIZE];

void cli_init(){
		// initialize the user functions
		init_user_functions();
}

void cli_execute( char *cmd, int max_size ){
		// Parse & execute the command stored into the 'str' (char str[80] ).
		// Delimiters are SPACE and COMA.
		char str[80];

		if( strlen(cmd)==0 )
			return;

		token_cnt = parse_string(cmd, tokens, " ,");
	  if (token_cnt <= 0)
			return;

		// loop through the user defined functions and execute
		// one if found
		for (int i = 0; i < MAX_USER_FUNCTIONS; i++) {
			debug_print( user_functions[i].command_name );
			//presumably end of user functions
			if (strcmp(user_functions[i].command_name, "") == 0)
				break;

			//display help
			if (strcmp(user_functions[i].command_name, tokens[0]) == 0) {
				if ((strcmp(tokens[1], "?") == 0) ||
						(strcmp(tokens[1], "--help") == 0)) {
					print_string( user_functions[i].command_help );
					return;
				} // end if
			}

			//list the user-defined functions
			if (strcmp("list", tokens[0]) == 0) {
				for (int j = 0; j < MAX_USER_FUNCTIONS; j++) {
						sprintf(str, "%s, ", user_functions[j].command_name);
						print_string(str);
				}
				return;
			}

			//execute the command function
			if (strcmp(user_functions[i].command_name, tokens[0]) == 0) {
				user_functions[i].user_function(tokens);
				//break;
				return;
			}
		} // eof for

		// user function not found
		sprintf(str, "%s ???", tokens[0]);
		print_string(str);
}



int parse_string(char *str, char tokens[][MAX_STRING_SIZE], char *delim) {
	// Transform a string into a list of tokens
  char *buffer = str;
  int x = 0;

  char *token = strtok(buffer, delim);
  while (token != NULL) {
    strcpy(tokens[x], token); // getting each token
    token = strtok(NULL, " ");
    x++;
  } // end while

  return x; // return number of tokens
}
