#ifndef USER_FUNCS_H    /* This is an "include guard" */
#define USER_FUNCS_H

#define NUMBER_OF_STRING 10
#define MAX_STRING_SIZE 25

#define MAX_USER_FUNCTIONS 5

typedef void (*user_func)(int token_count, char tokens[][MAX_STRING_SIZE]);

typedef  struct usr_funcs {
       char  command_name[20] ;
       user_func  user_function; // function pointer
			 char  command_help[50] ;
 } usr_funcs;


void register_user_functions();


// Should not be called directly outside of CLI
void calc(int token_count, char tokens[][MAX_STRING_SIZE]);
void cli_sd_info(int token_count, char tokens[][MAX_STRING_SIZE]);
void cli_dir(int token_count, char tokens[][MAX_STRING_SIZE]);
void cli_type( int token_count, char tokens[][MAX_STRING_SIZE]);
void cli_send_file( int token_count, char tokens[][MAX_STRING_SIZE]);

#endif /* USER_FUNCS_H */
