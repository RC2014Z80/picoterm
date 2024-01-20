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
#include "picoterm_core.h"
#include "picoterm_conio.h"
#include "picoterm_screen.h"
#include "../common/picoterm_config.h"
#include "../common/picoterm_debug.h"
#include "../common/picoterm_cursor.h"
#include "../common/picoterm_stddef.h"
#include "../common/picoterm_harddef.h"
#include "../common/keybd.h"
#include "../common/picoterm_i2c.h"
#include "../common/pca9536.h"
#include "../common/pio_sd.h"
#include "../cli/cli.h"
//#include "hardware/structs/bus_ctrl.h"
#include "bsp/board.h"
#include "tusb.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"


/* picoterm_cursor.c */
extern bool is_blinking;

/* picoterm_i2c.c */
extern i2c_inst_t *i2c_bus;
extern bool i2c_bus_available; // gp26 & gp27 are used as I2C (otherwise as simple GPIO)

// This is 4 for the font we're using
#define FRAGMENT_WORDS 4

static bool is_menu = false;   // switch between Terminal mode and Menu mode
static uint8_t id_menu = 0x00; // toggle with CTRL+SHIFT+M


//CU_REGISTER_DEBUG_PINS(frame_gen)
//CU_SELECT_DEBUG_PINS(frame_gen)

#define BTN_A 0
#define BTN_B 6
#define BTN_C 11

typedef bool (*render_scanline_func)(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_test_pattern(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_bg(struct scanvideo_scanline_buffer *dest, int core);


#define vga_mode vga_mode_640x480_60

#define COUNT 80

// for now we want to see second counter on native and don't need both cores

#define RENDER_ON_CORE1


render_scanline_func render_scanline = render_scanline_bg;

#define COORD_SHIFT 3
int vspeed = 1 * 1;
int hspeed = 1 << COORD_SHIFT;
int hpos;
int vpos;


static const int input_pin0 = 22;


// this is how a line of 8 bits is stored

uint32_t block[] = {
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0)
};



extern picoterm_config_t config; // Issue #13, awesome contribution of Spock64

extern const lv_font_t nupetscii_mono8; // Declare the available fonts
extern const lv_font_t cp437_mono8;
extern const lv_font_t nupetscii_olivetti_thin; // Declare the available fonts
extern const lv_font_t cp437_olivetti_thin;

const lv_font_t *font = &nupetscii_mono8;


// to make sure only one core updates the state when the frame number changes
// todo note we should actually make sure here that the other core isn't still rendering (i.e. all must arrive before either can proceed - a la barrier)
//auto_init_mutex(frame_logic_mutex);
struct mutex frame_logic_mutex;

static int left = 0;
static int top = 0;
static int x_sprites = 1;

void init_render_state(int core);
void led_blinking_task();
void usb_power_task();
void bell_task();

void render_loop() {
    /* Multithreaded execution */
    static uint8_t last_input = 0;
    static uint32_t last_frame_num = 0;
    int core_num = get_core_num();
    assert(core_num >= 0 && core_num < 2);
    //printf("Rendering on core %d\n", core_num);

    while (true) {
    scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);
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

#define TEST_WAIT_FOR_SCANLINE

#ifdef TEST_WAIT_FOR_SCANLINE
  volatile uint32_t scanline_color = 0;
#endif

uint8_t pad[65536]; // to check!!!

#define FONT_MAX_HEIGHT 15
#define FONT_WIDTH_WORDS FRAGMENT_WORDS
#define FONT_HEIGHT (font->line_height) // Should be identical accross all fonts.
#define FONT_MAX_SIZE_WORDS (FONT_MAX_HEIGHT * FONT_WIDTH_WORDS)
#define FONT_SIZE_WORDS (FONT_HEIGHT * FONT_WIDTH_WORDS)

uint32_t *font_raw_pixels = NULL;

void select_graphic_font( uint8_t font_id ){
  /* Assign GRAPHICAL font (nupetscii, cp437) by reassigning the `font` pointer */
  if(font_id == FONT_ASCII) // ignore for ANSI
    return;
  if(font_id == FONT_NUPETSCII_MONO8){
    font = &nupetscii_mono8;
    return;
  }
  if(font_id == FONT_CP437_MONO8){
    font = &cp437_mono8;
    return;
  }
	if(font_id == FONT_NUPETSCII_OLIVETTITHIN ){
		font = &nupetscii_olivetti_thin;
		return;
	}
	if(font_id == FONT_CP437_OLIVETTITHIN ){
		font = &cp437_olivetti_thin;
		return;
	}
}

void build_font( uint8_t font_id ){
    /* Build/fill internal structure for drawing font with PIO */
    uint16_t colors[16];
    // extended_font (NuPetScii) doesn't required inverted char
    // non extended_font (default font) just reduce the initial charset to 95 THEN compute the reverse value
    char max_char = font_id!=FONT_ASCII ? font->dsc->cmaps->range_length : 95;
    for (int i = 0; i < count_of(colors); i++) {
        colors[i] = PICO_SCANVIDEO_PIXEL_FROM_RGB5(1, 1, 1) * ((i * 3) / 2);
        if (i) i != 0x8000;
    }

    // We know range_length is 95 in the orginal font and full charset (up to 255)for the extended font
    // 4 is bytes per word, range_length is #chrs in font, FONT_SIZE_WORDS is words in width * font height

		if( font_raw_pixels == NULL )
    	font_raw_pixels = (uint32_t *) calloc(4, 256 * FONT_MAX_SIZE_WORDS * 2 );

    uint32_t *p = font_raw_pixels;
    uint32_t *pr = font_raw_pixels+( max_char * FONT_SIZE_WORDS); // pr is the reversed characters, build those in the same loop as the regular ones

    assert(font->line_height <= FONT_MAX_HEIGHT);

    for (int c = 0; c < max_char; c++) {
        // *** inefficient but simple ***

        // I don't fully understand this, hence the reverse is far from perfect.
        const lv_font_fmt_txt_glyph_dsc_t *g = &font->dsc->glyph_dsc[c + 1];
        const uint8_t *b = font->dsc->glyph_bitmap + g->bitmap_index;
        int bi = 0;

        for (int y = 0; y < FONT_HEIGHT; y++) {
          int ey = y - FONT_HEIGHT + font->base_line + g->ofs_y + g->box_h;
          for (int x = 0; x < FONT_WIDTH_WORDS * 2; x++) {
              uint32_t pixel;
              int ex = x - g->ofs_x;

              if (ex >= 0 && ex < g->box_w && ey >= 0 && ey < g->box_h) {
                  pixel = bi & 1 ? colors[b[bi >> 1] & 0xf] : colors[b[bi >> 1] >> 4];
                  bi++;
              } else {
                  pixel = 0;
              }

              // 201121  improved reverse video
              uint32_t r = 31 - PICO_SCANVIDEO_R5_FROM_PIXEL(pixel);
              uint32_t g = 31 - PICO_SCANVIDEO_G5_FROM_PIXEL(pixel);
              uint32_t b = 31 - PICO_SCANVIDEO_B5_FROM_PIXEL(pixel);

              switch (config.colour_preference) {
                case LIGHTAMBER:
                    b = 0;
                    g=g*0.8;
                    break;
                case DARKAMBER:
                    b = 0;
                    g=g*0.75;
                    break;
                case GREEN1: // g433n 1 33ff00
                    b = 0;
                    r=r*0.2;
                    break;
                case GREEN2: // g433n 1 00ff33
                    r = 0;
                    b=b*0.2;
                    break;
                case GREEN3: // g433n 1 00ff66
                    b = 0.4;
                    r=0;
                    break;
                case PURPLE: // g433n 1 00ff66
                    b = b*0.8;
                    r = r*0.9;
                    g = g*0.4;
                    break;
                default:
                  break;
              }

              uint32_t rvs_pixel = PICO_SCANVIDEO_PIXEL_FROM_RGB5(r,g,b);

              // 090622  adds colour options
              r = PICO_SCANVIDEO_R5_FROM_PIXEL(pixel);
              g = PICO_SCANVIDEO_G5_FROM_PIXEL(pixel);
              b = PICO_SCANVIDEO_B5_FROM_PIXEL(pixel);

              switch (config.colour_preference) {
                case LIGHTAMBER:
                    b = 0;
                    g=g*0.8;
                    break;
                case DARKAMBER:
                    b = 0;
                    g=g*0.75;
                    break;
                case GREEN1: // g433n 1 33ff00
                    b = 0;
                    r=r*0.2;
                    break;
                case GREEN2: // g433n 1 00ff33
                    r = 0;
                    b=b*0.2;
                    break;
                case GREEN3: // g433n 1 00ff66
                    b = 0.4;
                    r=0;
                    break;
                case PURPLE:
                    b = b*0.8;
                    r = r*0.9;
                    g = g*0.4;
                    break;
                default:
                    break;
              }

              uint32_t new_pixel = PICO_SCANVIDEO_PIXEL_FROM_RGB5(r,g,b);

              if (!(x & 1)) {
                  *p = new_pixel;
                   *pr = rvs_pixel;
              }
              else {
                  *p++ |= new_pixel << 16;
                  *pr++ |= rvs_pixel << 16;
              }
          } // for X

          if (ey >= 0 && ey < g->box_h)
              for (int x = FONT_WIDTH_WORDS * 2 - g->ofs_x; x < g->box_w; x++)
                  bi++;

        } // for Y
    } // for c
}

int video_main(void) {
    mutex_init(&frame_logic_mutex);
    select_graphic_font( config.font_id );
    build_font( config.font_id );
    sem_init(&video_setup_complete, 0, 1);

    setup_video();
    render_on_core1();   // render_loop() on core 1
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
    dest->fragment_words = FRAGMENT_WORDS;

    beginning_of_line[FRAGMENT_WORDS * 2 - 2] = COUNT * 2 * FRAGMENT_WORDS - 3 + 2;
    assert(FRAGMENT_WORDS * 2 == count_of(beginning_of_line));
    assert(FRAGMENT_WORDS * 2 == count_of(end_of_line));

    uint32_t *output32 = buf;
    *output32++ = host_safe_hw_ptr(beginning_of_line);
    uint32_t *dbase = font_raw_pixels + FONT_WIDTH_WORDS * (y % FONT_HEIGHT);
    int max_char = config.font_id>0 ?  font->dsc->cmaps->range_length : 95;

    char ch = 0;
    char inv = 0;
    char blk = 0;

    int tr = (y/FONT_HEIGHT);
    unsigned char *rowslots = slotsForRow(tr); // I want a better word for slots. (Character positions).
    unsigned char *rowinv = slotsForInvRow(tr);
    unsigned char *rowblk = slotsForBlkRow(tr);

    for (int i = 0; i < COUNT; i++) {
      ch = *rowslots;
      rowslots++;

      inv = *rowinv;
      rowinv++;

      blk = *rowblk;
      rowblk++;

      if(blk == 1 && is_blinking){
        if(inv == 1)
            *output32++ = host_safe_hw_ptr(dbase + ((max_char) * FONT_HEIGHT * FONT_WIDTH_WORDS));

        else
            *output32++ = host_safe_hw_ptr(&block);
      }
      else{
        if(inv == 1){
            *output32++ = host_safe_hw_ptr(dbase + ((ch + max_char) * FONT_HEIGHT * FONT_WIDTH_WORDS));
        }
        else{
          if(ch==0)
            *output32++ = host_safe_hw_ptr(&block);
            // shortcut
            // there's likely to be a lot of spaces on the screen.
            // if this character is a space, just use this predefined zero block rather than the calculation below

          else
            *output32++ = host_safe_hw_ptr(dbase + (ch * FONT_HEIGHT * FONT_WIDTH_WORDS));
          }
      }
    }

    *output32++ = host_safe_hw_ptr(end_of_line);
    *output32++ = 0; // end of chain

    assert(0 == (3u & (intptr_t) output32));
    assert((uint32_t *) output32 <= (buf + dest->data_max));

    dest->data_used = (uint16_t) (output32 - buf); // todo we don't want to include the off the end data in the "size" for the dma

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

void render_on_core1(){
  multicore_launch_core1(render_loop);
}

void stop_core1(){
  multicore_reset_core1();
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
    do {
        handle_new_character(read_key_from_buffer());
        // or for analysing what comes in
        //print_ascii_value(read_key_from_buffer());
    } while(key_ready());
    print_cursor();
  }
}

void usb_serial_task(){
  // Fill keyboard input buffer with char coming over the serial buffer
  //  (don't forget to make pico_enable_stdio_usb(picoterm 1)
  volatile int userInput = getchar_timeout_us (0);
  // 0xff if no character
  if(userInput!=PICO_ERROR_TIMEOUT){
    if (uart_is_writable(UART_ID)) {
      uart_putc (UART_ID, userInput);
    }
  }
}

//--------------------------------------------------------------------+
// MAIN
//--------------------------------------------------------------------+


int main(void) {
  debug_init(); // GPIO 22 as rx @ 115200
  debug_print( "main() - 80 column version" );


  gpio_init(LED);
  gpio_set_dir(LED, GPIO_OUT);
  gpio_put(LED,false);

  uint8_t bootchoice = 0;
  gpio_init(BTN_A);
  gpio_set_dir(BTN_A, GPIO_IN);
  gpio_pull_down(BTN_A);
  if(gpio_get (BTN_A)){
      bootchoice += 1;
  }
  gpio_init(BTN_B);
  gpio_set_dir(BTN_B, GPIO_IN);
  gpio_pull_down(BTN_B);
  if(gpio_get (BTN_B)){
      bootchoice += 2;
  }
  gpio_init(BTN_C);
  gpio_set_dir(BTN_C, GPIO_IN);
  gpio_pull_down(BTN_C);
  if(gpio_get (BTN_C)){
      bootchoice += 4;
  }

  switch (bootchoice) {
      case 0:
          // no button
          load_config();
          break;
      case 1:
          config.colour_preference = GREEN1;     // A
          save_config();
          break;
      case 2:
          config.colour_preference = DARKAMBER;  // B
          save_config();
          break;
      case 4:
          // C
          break;
      case 3:
          config.colour_preference = GREEN2;     // A+B
          save_config();
          break;
      case 5:
          config.colour_preference = GREEN3;     // C+A
          save_config();
          break;
      case 6:
          config.colour_preference = LIGHTAMBER; // C+B
          save_config();
          break;
      case 7:
                                           // A=B=C
          break;

      default:
          break;
  }

	// AFTER   reading and writing
  stdio_init_all();

  // Checking GP26 & GP27 will be handled as GPIO or I2C bus (with PCA9536 see issue #21)
  // Then initialize the IO for USB_POWER &
  i2c_bus_available = false;
  debug_print( "Check I2C capability" );
  init_i2c_bus(); // try to initialize the PicoTerm I2C bus
  if( has_pca9536( i2c_bus ) ){
    debug_print( "pca9536 detected!" );
    i2c_bus_available = true;
    pca9536_output_reset( i2c_bus, 0b0011 ); // preinitialize output at LOW
    pca9536_setup_io( i2c_bus, IO_0, IO_MODE_OUT ); // USB_POWER
    pca9536_setup_io( i2c_bus, IO_1, IO_MODE_OUT ); // BUZZER
    pca9536_setup_io( i2c_bus, IO_2, IO_MODE_IN ); // not used yet
    pca9536_setup_io( i2c_bus, IO_3, IO_MODE_IN ); // not used yet
  }
	// check other I2C GPIO expander here!

  if( i2c_bus_available )
    debug_print( "I2C bus detected" );

  if( !i2c_bus_available ){
    debug_print( "Using GPIO capability" );
    deinit_i2c_bus();

    gpio_init(USB_POWER_GPIO); // GPIO 26
    gpio_set_dir(USB_POWER_GPIO, GPIO_OUT);
    gpio_put(USB_POWER_GPIO,false);

    gpio_init(BUZZER_GPIO);
    gpio_set_dir(BUZZER_GPIO, GPIO_OUT);
    gpio_put(BUZZER_GPIO,false);
  }

  start_time = board_millis();

  uart_init(UART_ID, config.baudrate); // UART 1
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
  terminal_init();
	cli_init();
	spi_sd_init(); // Initialize pio_FatFS over PIO_SPI
	sd_mount(); // perform a mount test at boot

  video_main();       // also build the font
  terminal_reset();
  display_terminal(); // display terminal entry screen
  tusb_init(); // initialize tinyusb stack

  char _ch = 0;
  bool old_menu = false; // used to trigger when is_menu is changed

  while(true){
    // TinyUsb Host Task (see keybd.c::process_kdb_report() callback and pico_key_down() here below)
    tuh_task();
    usb_power_task();
    led_blinking_task();
    csr_blinking_task();
    key_repeat_task();
    bell_task();

    if( is_menu && !(old_menu) ){ // menu activated ?
      copy_main_to_secondary_screen(); // copy terminal screen
      save_cursor_position();
      clear_key_buffer(); // empty the keyboard buffer
      switch( id_menu ){
        case MENU_CONFIG:
          display_config();
          break;
        case MENU_CHARSET:
          display_charset();
          break;
        case MENU_HELP:
          display_help();
          break;
				case MENU_COMMAND:
					display_command();
					break;
      };
      old_menu = is_menu;
    }
    else if( !(is_menu) && old_menu ){ // menu de-activated ?
      //display_terminal();
      clrscr();
      copy_secondary_to_main_screen(); // restore terminal screen
      restore_cursor_position();
      clear_key_buffer(); // empty the keyboard buffer
      old_menu = is_menu;
    }

    if( is_menu ){ // Under menu display
      switch( id_menu ){
        case MENU_CONFIG:
          // Specialized handler manage keyboard input for menu
          _ch = handle_config_input();
          break;
				case MENU_COMMAND:
					// Specialized handler managing keyboard input for command
					_ch = handle_command_input();
					break;
        default:
          _ch = handle_default_input();
      } // eof Switch
      if( _ch==ESC ){ // ESC will also close the menu
          is_menu = false;
          id_menu = 0x00;
      }
    }
    else
      handle_keyboard_input(); // normal terminal management
  }
  return 0;
}

//--------------------------------------------------------------------+
//  Tasks
//--------------------------------------------------------------------+
bool led_state = false;
bool usb_power_state = false;

void led_blinking_task() {
  const uint32_t interval_ms_led = 1000;
  static uint32_t start_ms_led = 0;

  // No HID keyboard --> Led blink
  // HID Keyboard    --> Led off
  if( keyboard_attached() )
    board_led_write(false);
    //board_led_write(true);
  else {
      // Blink every interval ms
      if ( board_millis() - start_ms_led > interval_ms_led) {
        start_ms_led += interval_ms_led;

        board_led_write(led_state);
        led_state = 1 - led_state; // toggle
      }
  }
}

void usb_power_task() {
  if( !usb_power_state && ((board_millis() - start_time)>USB_POWER_DELAY )){
    usb_power_state = true;
    if( i2c_bus_available ){
      // USB_POWER wired on the IO_0 of PCA9536
      pca9536_output_io( i2c_bus, IO_0, true );
    }
    else
      // USB_POWER wired directly on the GPIO
      gpio_put( USB_POWER_GPIO, true );
  }
}



void bell_task() {
  const uint32_t interval_ms_bell = 100;
  static uint32_t start_ms_bell = 0;

  if(get_bell_state() == 1){
    start_ms_bell = board_millis();
    if(i2c_bus_available) // BuZZER wired on the IO_1 of PCA9536
      pca9536_output_io( i2c_bus, IO_1, true );
    else
      gpio_put(BUZZER_GPIO, true); // buzzer Wired directly on GPIO
    set_bell_state(2);
  }

  else if (get_bell_state() == 2 && board_millis() - start_ms_bell > interval_ms_bell) {
    if(i2c_bus_available)
      pca9536_output_io( i2c_bus, IO_1, false ); // BuZZER wired on the IO_1 of PCA9536
    else
      gpio_put(BUZZER_GPIO, false); // buzzer Wired directly on GPIO
    set_bell_state(0);
  }

}

//--------------------------------------------------------------------+
// keybd - Callback routines
//--------------------------------------------------------------------+

static void pico_key_down(int scancode, int keysym, int modifiers) {
		// sprintf( debug_msg, "pico_key_down scancode=%d ,keysym=%d ,modifiers=%d \r\n", scancode, keysym, modifiers );
		// debug_print( debug_msg );

  if( scancode_is_mod(scancode)==false ){
			// hotkey - Shortcut
			if( (scancode>=SCANCODE_F1) && (scancode<=SCANCODE_F12) && ((modifiers&WITH_SHIFT)==WITH_SHIFT) ){
				debug_print("Hotkey captured!");
				int f_key = (scancode-SCANCODE_F1)+1;
				char fname[20];
				sprintf( fname, "hotkey/f%d-%s.dat", f_key, ((modifiers|WITH_SHIFT)==WITH_SHIFT) ? "s" : "sc"  );
				send_file_to_uart( fname );
				return; // do not add key to "Keyboard buffer"
			}

      // which char at that key?
      uint8_t ch = keycode2ascii[scancode][0];
      // Is there a modifier key under use while pressing the key?
      if( (ch=='m') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        id_menu = MENU_CONFIG;
        is_menu = !(is_menu);
        return; // do not add key to "Keyboard buffer"
      }
      if( (ch=='n') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        id_menu = MENU_CHARSET;
        is_menu = !(is_menu);
        return; // do not add key to "Keyboard buffer"
      }

      if( (ch=='h') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        id_menu = MENU_HELP;
        is_menu = !(is_menu);
        return; // do not add key to "Keyboard buffer"
      }

      if( (ch=='l') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        // toggle between graphical font and ANSI font
        config.font_id = (config.font_id == 0 ? config.graph_id : 0);
        build_font( config.font_id );
        return; // do not add key to "Keyboard buffer"
      }

      if( (ch=='c') && (modifiers == (WITH_CTRL + WITH_SHIFT)) ){
        id_menu = MENU_COMMAND;
        is_menu = !(is_menu);
        return; // do not add key to "Keyboard buffer"
      }

      // Is this a scancode with special Escape Sequence Attached
      signed char idx = scancode_has_esc_seq(scancode);
      if ( !(is_menu) && (idx>-1) ){
        // debug_print("has esc sequence!");
        for( char k=0; k < scancode_esc_seq_len(idx); k++)
          uart_putc( UART_ID, scancode_esc_seq_item(idx,k) );
        return;
      }

      if( modifiers & WITH_SHIFT ){
          ch = keycode2ascii[scancode][1];
      }
      else if((modifiers & WITH_CAPSLOCK) && ch>='a' && ch<='z'){
          ch = keycode2ascii[scancode][1];
      }

      if((modifiers & WITH_CTRL) && ch>63 && ch<=95){ // A..Z  ord(A)=65
          ch=ch-64;
      }
      else if((modifiers & WITH_CTRL) && ch>95){  // a..z  ord(a)=97
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
      else {
         uart_putc (UART_ID, ch);
      }
    }
}

static void pico_key_up(int scancode, int keysym, int modifiers) {
   printf("Key up, %i, %i, %i \r\n", scancode, keysym, modifiers);
}
