#ifndef USER_FUNCS_H    /* This is an "include guard" */
#define USER_FUNCS_H

#define NUMBER_OF_STRING 10
#define MAX_STRING_SIZE 25

#define MAX_USER_FUNCTIONS 5

typedef void (*user_func)(char tokens[][MAX_STRING_SIZE]);

typedef  struct usr_funcs {
       char  command_name[20] ;
       user_func  user_function; // function pointer
			 char  command_help[50] ;
 } usr_funcs;


void init_user_functions();


#endif /* USER_FUNCS_H */
