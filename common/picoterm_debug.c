#include "picoterm_harddef.h" // Hardware definition
#include "picoterm_debug.h"
#include "uart_tx.pio.h"
#include <stdio.h>
#include <stdarg.h>

static PIO pio = pio1;
static uint sm = 0;
static uint offset = (uint)NULL;

void debug_init(){
	offset = pio_add_program(pio, &uart_tx_program);
	uart_tx_program_init(pio, sm, offset, DEBUG_TX, DEBUG_BAUD);
	uart_tx_program_puts(pio, sm, "\r\n\r\nPicoTerm Debug UART initialized\r\n");
}

void debug_print( const char *s ){
	uart_tx_program_puts(pio, sm, s );
	uart_tx_program_puts(pio, sm, "\r\n");
}

void debug_write( const char *s ){
	uart_tx_program_puts(pio, sm, s );
}
