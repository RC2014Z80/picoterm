#ifndef _PICOTERM_DEBUG_H_
#define _PICOTERM_DEBUG_H_

#include "hardware/pio.h"

static char debug_msg[100];

void debug_init();
void debug_print( const char *s ); // ends message with CR/LF
void debug_write( const char *s ); // just write the bytes

#endif // _PICOTERM_DEBUG_H_
