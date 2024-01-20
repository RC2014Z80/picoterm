#ifndef _KEYB_D_H_
#define _KEYB_D_H_

#include "stdint.h"
#include "stdbool.h"
#include "pmhid.h"
//#include "common/tusb_common.h"

#define WITH_SHIFT 0x8000
#define WITH_ALTGR 0x4000
#define WITH_CTRL 0x2000
#define WITH_CAPSLOCK 0x1000

// Keyboard device USB address
#define UNDEFINED_ADDR  0xFF
static uint8_t keybd_dev_addr = UNDEFINED_ADDR;

// Conversion ScanCode -> Ascii
static uint8_t const keycode2ascii[128][3] =  { PM_KEYCODE_TO_ASCII };
bool scancode_is_mod(int scancode);
static uint8_t const keycode2escseq[PM_ESC_SEQ_COUNT][PM_ESC_SEQ_MAX_LEN+2] =  { PM_KEYCODE_TO_ESC_SEQ };
signed char scancode_has_esc_seq(int scancode); // index of escape sequence corresponding to ScanCode (like Key right, Key Left) or -1
int scancode_esc_seq_len(uint8_t index);        // len of chars in the Escape Sequence
char scancode_esc_seq_item(uint8_t index, uint8_t pos ); // One of the char of the escape sequence

// Key down/up callbacks
typedef void (*key_change_cb_t)( int scancode, int keysym, int modifiers );
static key_change_cb_t key_down_cb = NULL;
static key_change_cb_t key_up_cb = NULL;

// Exported function
void keybd_init( key_change_cb_t key_down_callback, key_change_cb_t key_up_callback );
bool keyboard_attached();
void key_repeat_task();

void insert_key_into_buffer(unsigned char ch);
bool key_ready();
unsigned char read_key_from_buffer();
void clear_key_buffer();

#endif
