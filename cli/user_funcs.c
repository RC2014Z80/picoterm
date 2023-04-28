#include "user_funcs.h"
#include "pico/stdlib.h"
#include "tinyexpr.h"
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include "../common/picoterm_stdio.h"

usr_funcs user_functions[MAX_USER_FUNCTIONS];

// early declaration
void calc(char tokens[][MAX_STRING_SIZE]);
void blink_led(char tokens[][MAX_STRING_SIZE]);
void read_pir_sensor(char tokens[][MAX_STRING_SIZE]);

void init_user_functions() {
	// Each user function you create, you must also
	// add to the init_user_functions routine.


  // This is my first user-defined function. I give
  // the command a name - in this case "blink_led".
  // You can supply a short help description. And,
  // then, I point the user_functions struct array
  // to the actual name of my user-defined function.
  strcpy(user_functions[0].command_name, "blink_led");
  strcpy(user_functions[0].command_help, "blink_led pin_number,iterations,duration (ms)");
  user_functions[0].user_function = blink_led;

  // Here is a second user function.
  strcpy(user_functions[1].command_name, "read_pir_sensor");
  strcpy(user_functions[1].command_help, "read_pir_sensor,pin_number,iterations");
  user_functions[1].user_function = read_pir_sensor;

  //Here is a third user function.
  strcpy(user_functions[2].command_name, "calc");
  strcpy(user_functions[2].command_help, "calc math_expression");
  user_functions[2].user_function = calc;
}

//--------------------------------------------------------------------+
//  calc function
//--------------------------------------------------------------------+

void calc(char tokens[][MAX_STRING_SIZE]) {
	// This is using the TinyExprMath Expression Parser
	// found at: https://github.com/codeplea/tinyexpr
  char expr[80];
  char answer[10];

  strcpy(expr, tokens[1]);

  if (strlen(expr) <= 1) {
    print_string("Not a math_expression.\r\n");
    return;
  }

  sprintf(answer, "%f\r\n", te_interp(expr, 0)); /* Prints 25. */
  print_string(answer);

} // end calcware/uart.h"


//--------------------------------------------------------------------+
//  xxxxxxxxxxxx
//--------------------------------------------------------------------+

void blink_led(char tokens[][MAX_STRING_SIZE]) {
  // uint8_t LED_PIN = atoi(tokens[1]);
  // uint16_t iters = atoi(tokens[2]);
  // uint16_t millis = atoi(tokens[3]);
}

//--------------------------------------------------------------------+
//  xxxxxxxxxxxxxxx
//--------------------------------------------------------------------+

void read_pir_sensor(char tokens[][MAX_STRING_SIZE]) {
  // uint8_t PIR_PIN = atoi(tokens[1]);
  // uint16_t iters = atoi(tokens[2]);
}
