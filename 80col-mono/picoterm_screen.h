/* ==========================================================================
        Manage the screen rendering :
              * display_x functions &
              * keyboard callback function for keyboard
   ========================================================================== */


#ifndef _PICOTERM_SCREEN_H
#define _PICOTERM_SCREEN_H

char handle_default_input();

void display_terminal();

void display_command(); // command line console
char handle_command_input();

void display_charset();

void display_config();
char handle_config_input();

void display_help();



#endif
