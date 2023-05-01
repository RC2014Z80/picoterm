/* ==========================================================================
    ConIO configuration accessible to the application & private picoterm_conio.c
   ========================================================================== */

#include "picoterm_conio_config.h"
#include "picoterm_dec.h"
#include <stdbool.h>

picoterm_conio_config_t conio_config  = { .rvs = false, .blk = false, .just_wrapped = false,
    .wrap_text = true, .dec_mode = DEC_MODE_NONE, .cursor.pos.x = 0, .cursor.pos.y = 0,
    .cursor.state.visible = true, .cursor.state.blink_state = false,
    .cursor.state.blinking_mode = true, .cursor.symbol = 143 };


void conio_config_init(){
		conio_config.rvs = false;
	  conio_config.blk = false;
	  conio_config.wrap_text = true;
	  conio_config.just_wrapped = false;
	  conio_config.dec_mode = DEC_MODE_NONE; // single/double lines

		conio_config.cursor.state.visible = true;
	  conio_config.cursor.state.blink_state = false; // blinking cursor is in hidden state
	  conio_config.cursor.state.blinking_mode = true;
	  conio_config.cursor.symbol = 143;
}
