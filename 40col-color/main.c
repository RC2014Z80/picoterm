/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 &21
 * Shiela Dixon, https://peacockmedia.software
 *
 * much of what's in this main file is taken from the VGA textmode example
 * from pico-playground/scanvideo which has the licence as follows:
 *
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *
 * ... and the TinyUSB hid_app, which has the following licence:
 *
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *
 *
 * picoterm.c handles the behaviour of the terminal and storing the text
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
 /* MCHobby notes:
    - render on core 1. see #define RENDER_ON_CORE1
 */

#include "main.h"
#include "picoterm.h"
#include "../common/picoterm_config.h"
#include "../common/picoterm_debug.h"
#include "../common/keybd.h"

#include "bsp/board.h"
#include "tusb.h"

// This is 4 for the font we're using
#define FRAGMENT_WORDS 4

static bool is_menu = false; // toggle with CTRL+SHIFT+M

typedef bool (*render_scanline_func)(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_test_pattern(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_bg(struct scanvideo_scanline_buffer *dest, int core);


//#define vga_mode vga_mode_640x480_60
#define vga_mode vga_mode_320x240_60
//#define vga_mode vga_mode_213x160_60
//#define vga_mode vga_mode_160x120_60
////#define vga_mode vga_mode_tft_800x480_50
//#define vga_mode vga_mode_tft_400x240_50

//#define COUNT ((vga_mode.width/8)-1)  // why one less?
#define COUNT (vga_mode.width/8)
// #define COUNT COLUMNS      // ideally

// for now we want to see second counter on native and don't need both cores

// todo there is a bug in multithreaded rendering atm
//#define RENDER_ON_CORE0
#define RENDER_ON_CORE1
//#define IRQS_ON_CORE1

render_scanline_func render_scanline = render_scanline_bg;

#define COORD_SHIFT 3
int vspeed = 1 * 1;
int hspeed = 1 << COORD_SHIFT;
int hpos;
int vpos;

static const int input_pin0 = 22;


// this is how a line of 8 bits is stored

uint32_t block[] = {
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(31,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0)
};


// to make sure only one core updates the state when the frame number changes
// todo note we should actually make sure here that the other core isn't still rendering (i.e. all must arrive before either can proceed - a la barrier)
//auto_init_mutex(frame_logic_mutex);
struct mutex frame_logic_mutex;

static int left = 0;
static int top = 0;
static int x_sprites = 1;

void init_render_state(int core);
void led_blinking_task(void);


void render_loop() {
    /* Multithreaded execution */
    static uint8_t last_input = 0;
    static uint32_t last_frame_num = 0;
    int core_num = get_core_num();
    assert(core_num >= 0 && core_num < 2);
    //printf("Rendering on core %d\n", core_num);

    while (true) {
        struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);
//        if (scanline_buffer->data_used) {
//            // validate the previous scanline to make sure noone corrupted it
//            validate_scanline(scanline_buffer->data, scanline_buffer->data_used, vga_mode.width, vga_mode.width);
//        }
        // do any frame related logic
        // todo probably a race condition here ... thread dealing with last line of a frame may end
        // todo up waiting on the next frame...

        mutex_enter_blocking(&frame_logic_mutex);
        uint32_t frame_num = scanvideo_frame_number(scanline_buffer->scanline_id);
        // note that with multiple cores we may have got here not for the first scanline, however one of the cores will do this logic first before either does the actual generation
        if (frame_num != last_frame_num) {
            // this could should be during vblank as we try to create the next line
            // todo should we ignore if we aren't attempting the next line
            last_frame_num = frame_num;
            hpos += hspeed;
//            if (hpos < 0) {
//                hpos = 0;
//                hspeed = -hspeed;
//            } else if (hpos >= (level0_map_width*8 - vga_mode.width) << COORD_SHIFT) {
//                hpos = (level0_map_width*8 - vga_mode.width) << COORD_SHIFT;
//                hspeed = -hspeed;
//            }
//            uint8_t new_input = gpio_get(input_pin0);
//            if (last_input && !new_input) {
//                static int foo = 1;
//                foo++;
//
//               bus_ctrl_hw->priority = (foo & 1u) << BUSCTRL_BUS_PRIORITY_DMA_R_LSB;

//                hpos++;
//            }
//            last_input = new_input;
//            static int bar = 1;

        }
        mutex_exit(&frame_logic_mutex);
        //DEBUG_PINS_SET(frame_gen, core_num ? 2 : 4);
        render_scanline(scanline_buffer, core_num);
        //DEBUG_PINS_CLR(frame_gen, core_num ? 2 : 4);
#if PICO_SCANVIDEO_PLANE_COUNT > 2
        assert(false);
#endif
        // release the scanline into the wild
        scanvideo_end_scanline_generation(scanline_buffer);
        // do this outside mutex and scanline generation


    } // end while(true) loop
}

struct semaphore video_setup_complete;

void setup_video() {
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_setup_complete);
}

void core1_func() {
    render_loop();
}

#define TEST_WAIT_FOR_SCANLINE

#ifdef TEST_WAIT_FOR_SCANLINE
volatile uint32_t scanline_color = 0;
#endif

uint8_t pad[65536];


uint32_t *font_raw_pixels;


#define FONT_WIDTH_WORDS FRAGMENT_WORDS
#if FRAGMENT_WORDS == 5
const lv_font_t *font = &ubuntu_mono10;
//const lv_font_t *font = &lcd;
#elif FRAGMENT_WORDS == 4
//const lv_font_t *font = &ubuntu_mono8;
#else
//const lv_font_t *font = &ubuntu_mono6;
#endif
#define FONT_HEIGHT 8
#define FONT_SIZE_WORDS (FONT_HEIGHT * FONT_WIDTH_WORDS)




int video_main(void) {


    mutex_init(&frame_logic_mutex);

    sem_init(&video_setup_complete, 0, 1);

    setup_video();

    init_render_state(0);   // currently does nothing
    init_render_state(1);   // currently oes nothing


#ifdef RENDER_ON_CORE1
    render_on_core1();  // render_loop() on core 1
#endif
#ifdef RENDER_ON_CORE0
    render_loop();
#endif

}

void render_on_core1(){
	multicore_launch_core1(render_loop);
}

void stop_core1(){
	multicore_reset_core1();
}

// must not be called concurrently
void init_render_state(int core) {


}


static __not_in_flash("x") uint16_t beginning_of_line[] = {
        // todo we need to be able to shift scanline to absorb these extra pixels
#if FRAGMENT_WORDS == 5
        COMPOSABLE_RAW_1P, 0,
#endif
#if FRAGMENT_WORDS >= 4
        COMPOSABLE_RAW_1P, 0,
#endif
        COMPOSABLE_RAW_1P, 0,
        // main run, 2 more black pixels
        COMPOSABLE_RAW_RUN, 0,
        0/*COUNT * 2 * FRAGMENT_WORDS -3 + 2*/, 0
};
static __not_in_flash("y") uint16_t end_of_line[] = {
#if FRAGMENT_WORDS == 5 || FRAGMENT_WORDS == 3
        COMPOSABLE_RAW_1P, 0,
#endif
#if FRAGMENT_WORDS == 3
        COMPOSABLE_RAW_1P, 0,
#endif
#if FRAGMENT_WORDS >= 4
        COMPOSABLE_RAW_2P, 0,
        0, COMPOSABLE_RAW_1P_SKIP_ALIGN,
        0, 0,
#endif
        COMPOSABLE_EOL_SKIP_ALIGN, 0xffff // eye catcher
};


bool render_scanline_bg(struct scanvideo_scanline_buffer *dest, int core) {
    // 1 + line_num red, then white
    uint32_t *buf = dest->data;
    size_t buf_length = dest->data_max;
    int y = scanvideo_scanline_number(dest->scanline_id) + vpos;
    int x = hpos;

    // we handle both ends separately
//    static const uint32_t end_of_line[] = {
//            COMPOSABLE_RAW_1P | (0u<<16),
//            COMPOSABLE_EOL_SKIP_ALIGN | (0xffff << 16) // eye catcher ffff
//    };

//#undef COUNT
//#define COUNT MIN(vga_mode.width/(FRAGMENT_WORDS*2)-1, 40)   // we've defined it as 40, want 40


    dest->fragment_words = FRAGMENT_WORDS;

    beginning_of_line[FRAGMENT_WORDS * 2 - 2] = COUNT * 2 * FRAGMENT_WORDS - 3 + 2;
    assert(FRAGMENT_WORDS * 2 == count_of(beginning_of_line));
    assert(FRAGMENT_WORDS * 2 == count_of(end_of_line));

    uint32_t *output32 = buf;

    *output32++ = host_safe_hw_ptr(beginning_of_line);
    uint32_t *dbase = font_raw_pixels + FONT_WIDTH_WORDS * (y % FONT_HEIGHT);


    char ch = 0;

    int tr = y; // (y/FONT_HEIGHT);
    uint32_t *rowwords = wordsForRow(tr);

    for (int i = 0; i < COUNT; i++) {

          *output32++ = host_safe_hw_ptr(rowwords);

          rowwords+=4;      // basically every output32 assignment is a pointer to 4 words / 8 pixels


    }


    *output32++ = host_safe_hw_ptr(end_of_line);
    *output32++ = 0; // end of chain

    assert(0 == (3u & (intptr_t) output32));
    assert((uint32_t *) output32 <= (buf + dest->data_max));

    dest->data_used = (uint16_t) (output32 -
                                  buf); // todo we don't want to include the off the end data in the "size" for the dma



#if PICO_SCANVIDEO_PLANE_COUNT > 1
#if !PICO_SCANVIDEO_PLANE2_VARIABLE_FRAGMENT_DMA
    assert(false);
#endif
    buf = dest->data2;
    output32 = buf;

    uint32_t *inline_data = output32 + PICO_SCANVIDEO_MAX_SCANLINE_BUFFER2_WORDS / 2;
    output = (uint16_t *)inline_data;

    uint32_t *base = (uint32_t *)output;

#define MAKE_SEGMENT \
    assert(0 == (3u & (intptr_t)output)); \
    *output32++ = (uint32_t*)output - base; \
    *output32++ = host_safe_hw_ptr(base); \
    base = (uint32_t *)output;

    int wibble = (frame_number(dest->scanline_id)>>2)%7;
    for(int q = 0; q < x_sprites; q++) {
        // nice if we can do two black pixel before
        *output++ = COMPOSABLE_RAW_RUN;
        *output++ = 0;
        *output++ = galaga_tile_data.width + 2 - 3;
        *output++ = 0;
        MAKE_SEGMENT;

        span_offsets = galaga_tile_data.span_offsets + (q+wibble) * galaga_tile_data.height + (y - vpos);//(y%galaga_tile_data.count 7u);
        off = span_offsets[0];
        data = (uint16_t *) (galaga_tile_data.blob.bytes + off);

        *output32++ = galaga_tile_data.width / 2;
        *output32++ = host_safe_hw_ptr(data);
    }
    *output++ = COMPOSABLE_RAW_1P;
    *output++ = 0;
    *output++ = COMPOSABLE_EOL_SKIP_ALIGN;
    *output++ = 0xffff;
    MAKE_SEGMENT;

    // end of dma chain
    *output32++ = 0;
    *output32++ = 0;

    assert(output32 < inline_data);
    assert((uint32_t*)output <= (buf + dest->data2_max));
    dest->data2_used = (uint16_t)(output32 - buf); // todo we don't want to include the inline data in the "size" for the dma
#endif
    dest->status = SCANLINE_OK;



    return true;
}

void go_core1(void (*execute)()) {
    multicore_launch_core1(execute);
}


void on_uart_rx() {
  // we can buffer the character here if we turn on interrupts for UART

  // FIFO turned off, we should be here once for each character,
  // but the while does no harm and at least acts as an if()
  while (uart_is_readable (UART_ID)){
    insert_key_into_buffer(uart_getc (UART_ID));
  }

}


void tih_handler(){
    gpio_put(LED,true);
}


void handle_keyboard_input(){
  // normal terminal operation: if key received -> display it on term
  if(key_ready()){
    clear_cursor();

    do{

        handle_new_character(read_key_from_buffer());
        // or for analysing what comes in
        // print_ascii_value(cpmInput);

    }while(key_ready());

    print_cursor();
  }

}

//--------------------------------------------------------------------+
// MAIN
//--------------------------------------------------------------------+

int main(void) {
  debug_init(); // GPIO 28 as rx @ 115200
  debug_print( "main() 40 column version" );

  gpio_init(LED);
  gpio_set_dir(LED, GPIO_OUT);
  gpio_put(LED,false);

  stdio_init_all();
  load_config();
  tusb_init(); // initialize tinyusb stack


  uart_init(UART_ID, config.baudrate);
  uart_set_hw_flow(UART_ID,false,false);
  uart_set_format(UART_ID, config.databits, config.stopbits, config.parity);

	// Set the TX and RX pins by using the function select on the GPIO
	// Set datasheet for more information on function select
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

	// This should enable rx interrupt handling
	// Turn off FIFO's - we want to do this character by character
  uart_set_fifo_enabled(UART_ID, false);
  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

  // set up and enable the interrupt handlers
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);

  // enable the UART
  uart_set_irq_enables(UART_ID, true, false);

	// Initialise keyboard module
	keybd_init( pico_key_down, pico_key_up );

	prepare_text_buffer();
	display_terminal(); // display terminal entry screen
	video_main();
	tusb_init(); // initialize tinyusb stack

	char _ch = 0;
	bool old_menu = false; // used to trigger when is_menu is changed

	while(true){
		// TinyUsb Host Task (see keybd.c:process_kdb_report() callback and pico_key_down() here below)
		tuh_task();
		led_blinking_task();

		if( is_menu && !(old_menu) ){ // CRL+M : menu activated ?
			// empty the keyboard buffer
			while( key_ready() )
				read_key_from_buffer();
			display_menu();
			old_menu = is_menu;
		}
		else if( !(is_menu) && old_menu ){ // CRL+M : menu de-activated ?
			display_terminal();
			old_menu = is_menu;
		}

		if( is_menu ){ // Under menu display
			_ch = handle_menu_input(); // manage keyboard input for menu
			if( _ch==ESC ) // ESC will also close the menu
					is_menu = false;
		}
		else
			handle_keyboard_input(); // normal terminal management
	} // while(true)
	return 0;
}

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
bool led_state = false;

void led_blinking_task(void) {
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

	// No HID keyboard --> Led blink
	// HID Keyboard    --> Led off
	if( keyboard_attached() )
		board_led_write(false);
    //board_led_write(true);
  else {
      // Blink every interval ms
      if ( board_millis() - start_ms > interval_ms) {
        start_ms += interval_ms;

        board_led_write(led_state);
        led_state = 1 - led_state; // toggle
      }
  }
}

//--------------------------------------------------------------------+
// keybd - Callback routines
//--------------------------------------------------------------------+

static void pico_key_down(int scancode, int keysym, int modifiers) {
    //printf("Key down, %i, %i, %i \r\n", scancode, keysym, modifiers);

    if( scancode_is_mod(scancode)==false ){
      // which char at that key?
      uint8_t ch = keycode2ascii[scancode][0];
      // Is there a modifier key under use while pressing the key?
      if( (ch=='m') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        is_menu = !(is_menu);
        return; // do not add key to "Keyboard buffer"
      }
      if( modifiers & WITH_SHIFT ){
          ch = keycode2ascii[scancode][1];
      }
      else if((modifiers & WITH_CAPSLOCK) && ch>='a' && ch<='z'){
          ch = keycode2ascii[scancode][1];
      }
      else if(modifiers & WITH_CTRL && ch>95){
          ch=ch-96;
      }
      else if(modifiers & WITH_ALTGR){
          ch = keycode2ascii[scancode][2];
      }
      //printf("Character: %c\r\n", ch);
      // foward key-pressed to UART (only when typing in the terminal)
      // otherwise, send it directly to the keyboard buffer
      if( is_menu )
        insert_key_into_buffer( ch );
      else
         uart_putc (UART_ID, ch);
    }
}

static void pico_key_up(int scancode, int keysym, int modifiers) {
   printf("Key up, %i, %i, %i \r\n", scancode, keysym, modifiers);
}
