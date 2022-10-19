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
//#include "hardware/structs/bus_ctrl.h"

#define LED             25
#define UART_ID         uart1   // also see hid_app.c
#define UART_TX_PIN     20
#define UART_RX_PIN     21
#define CLOCK_SPEED     133000
#define BAUD_RATE       115200 // /(CLOCK_SPEED/133000)
#define DATA_BITS       8
#define STOP_BITS       1
#define PARITY          UART_PARITY_NONE

// This is 4 for the font we're using
#define FRAGMENT_WORDS 4

int LED_status;  // 0 off, 1 on, 2 flashing
bool led_state = false;
static bool is_menu = false; // toggle with CTRL+SHIFT+M

//CU_REGISTER_DEBUG_PINS(frame_gen)
//CU_SELECT_DEBUG_PINS(frame_gen)


#define BTN_A 0
#define BTN_B 6
#define BTN_C 11



#define FLASH_TARGET_OFFSET (256 * 1024)  // from start of flash

// once written, we can access our data at flash_target_contents
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);





////////////////////////////
// new TinyUSB stuff


enum {
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226, /**< alt, option */
    SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230, /**< alt gr, option */
    SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */
};

#define WITH_SHIFT 0x8000
#define WITH_ALTGR 0x4000
#define WITH_CTRL 0x2000
#define WITH_CAPSLOCK 0x1000



typedef bool (*render_scanline_func)(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_test_pattern(struct scanvideo_scanline_buffer *dest, int core);
bool render_scanline_bg(struct scanvideo_scanline_buffer *dest, int core);


// a buffer for characters coming in from UART. It's of limited value as things stand
// but will come into its own if and when we switch to interrupts for UART
struct KeyboardBuffer {
  int length;
  int take;
  int insert;
  char buff[2000];
};

struct KeyboardBuffer keybuffer1 = {0};

void insert_key_into_buffer(unsigned char ch){
  keybuffer1.buff[keybuffer1.insert++]=ch;
  if(keybuffer1.insert==keybuffer1.length)keybuffer1.insert=0;
}

bool key_ready(){
    return (keybuffer1.take!=keybuffer1.insert);
}

unsigned char read_key_from_buffer(){
    unsigned char ch=keybuffer1.buff[keybuffer1.take++];
    if(keybuffer1.take==keybuffer1.length)keybuffer1.take=0;
    return ch;
}


#define vga_mode vga_mode_640x480_60
//#define vga_mode vga_mode_320x240_60
//#define vga_mode vga_mode_213x160_60
//#define vga_mode vga_mode_160x120_60
////#define vga_mode vga_mode_tft_800x480_50
//#define vga_mode vga_mode_tft_400x240_50

#define COUNT ((vga_mode.width/8)-1)

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
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0),
                    PICO_SCANVIDEO_PIXEL_FROM_RGB5(0,0,0) << 16 |
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



void write_data_to_flash(){
  /* unsafe if you have two cores concurrently executing from flash
     https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__flash.html
  */

    uint8_t data_to_write[FLASH_PAGE_SIZE];
    data_to_write[0] = colour_preference;

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, data_to_write, FLASH_PAGE_SIZE);

}

void read_data_from_flash(){
    colour_preference = flash_target_contents[0];
}




// ok this is going to be the beginning of retained mode
//


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
const lv_font_t *font = &ubuntu_mono8;
#else
const lv_font_t *font = &ubuntu_mono6;
#endif
#define FONT_HEIGHT (font->line_height)
#define FONT_SIZE_WORDS (FONT_HEIGHT * FONT_WIDTH_WORDS)

void build_font() {
    uint16_t colors[16];
    for (int i = 0; i < count_of(colors); i++) {
        colors[i] = PICO_SCANVIDEO_PIXEL_FROM_RGB5(1, 1, 1) * ((i * 3) / 2);
        if (i) i != 0x8000;
    }

    // 4 is bytes per word, range_length is #chrs in font, FONT_SIZE_WORDS is words in width * font height
    font_raw_pixels = (uint32_t *) calloc(4, font->dsc->cmaps->range_length * FONT_SIZE_WORDS * 2);

    uint32_t *p = font_raw_pixels;
    // we know range_length is 95
    uint32_t *pr = font_raw_pixels+(font->dsc->cmaps->range_length * FONT_SIZE_WORDS);
    // pr is the reversed characters, build those in the same loop as the regular ones

    assert(font->line_height == FONT_HEIGHT);

    // our range_length is 95
    for (int c = 0; c < (font->dsc->cmaps->range_length); c++) {
        // inefficient but simple

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

            switch (colour_preference)
            {
            case LIGHTAMBER:
            // light amber
                b = 0;
                g=g*0.8;
                break;
            case DARKAMBER:
            // dark amber
                b = 0;
                g=g*0.75;
                break;
            case GREEN1:
            // g433n 1 33ff00
                b = 0;
                r=r*0.2;
                break;
            case GREEN2:
            // g433n 1 00ff33
                r = 0;
                b=b*0.2;
                break;
            case GREEN3:
            // g433n 1 00ff66
                b = 0.4;
                r=0;
                break;
            default:
            break;
            }

            uint32_t rvs_pixel = PICO_SCANVIDEO_PIXEL_FROM_RGB5(r,g,b);


              // 090622  adds colour options
              r = PICO_SCANVIDEO_R5_FROM_PIXEL(pixel);
              g = PICO_SCANVIDEO_G5_FROM_PIXEL(pixel);
              b = PICO_SCANVIDEO_B5_FROM_PIXEL(pixel);


            switch (colour_preference)
            {
            case LIGHTAMBER:
                // light amber
                b = 0;
                g=g*0.8;
                break;
            case DARKAMBER:
                // dark amber
                b = 0;
                g=g*0.75;
                break;
            case GREEN1:
                // g433n 1 33ff00
                b = 0;
                r=r*0.2;
                break;
            case GREEN2:
                // g433n 1 00ff33
                r = 0;
                b=b*0.2;
                break;
            case GREEN3:
                // g433n 1 00ff66
                b = 0.4;
                r=0;
                break;
            default:
                break;
            }


            uint32_t new_pixel = PICO_SCANVIDEO_PIXEL_FROM_RGB5(r,g,b);



              if (!(x & 1)) {
                  *p = new_pixel;
                  // *pr = ~pixel;
                  *pr = rvs_pixel;
              } else {
                  *p++ |= new_pixel << 16;
                  //*pr++ |= ~pixel << 16;
                  *pr++ |= rvs_pixel << 16;
              }

            }
            if (ey >= 0 && ey < g->box_h) {
                for (int x = FONT_WIDTH_WORDS * 2 - g->ofs_x; x < g->box_w; x++) {
                    bi++;
                }
            }
        }
    }
}

int video_main(void) {


    mutex_init(&frame_logic_mutex);


    build_font();
    sem_init(&video_setup_complete, 0, 1);

    setup_video();

    init_render_state(0);   // does nothing
    init_render_state(1);   // does nothing


#ifdef RENDER_ON_CORE1
    render_on_core1();   // render_loop() on core 1
#endif
#ifdef RENDER_ON_CORE0
    render_loop();
#endif

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
#undef COUNT
    // todo for SOME REASON, 80 is the max we can do without starting to really get bus delays (even with priority)... not sure how this could be
    // todo actually it seems it can work, it just mostly starts incorrectly synced!?
#define COUNT MIN(vga_mode.width/(FRAGMENT_WORDS*2)-1, 80)

    dest->fragment_words = FRAGMENT_WORDS;

    beginning_of_line[FRAGMENT_WORDS * 2 - 2] = COUNT * 2 * FRAGMENT_WORDS - 3 + 2;
    assert(FRAGMENT_WORDS * 2 == count_of(beginning_of_line));
    assert(FRAGMENT_WORDS * 2 == count_of(end_of_line));

    uint32_t *output32 = buf;

    *output32++ = host_safe_hw_ptr(beginning_of_line);
    uint32_t *dbase = font_raw_pixels + FONT_WIDTH_WORDS * (y % FONT_HEIGHT);
    int cmax = font->dsc->cmaps[0].range_length;



    char ch = 0;

    int tr = (y/FONT_HEIGHT);
    unsigned char *rowslots = slotsForRow(tr); // I want a better word for slots. (Character positions).

    for (int i = 0; i < COUNT; i++) {

      ch = *rowslots;
      rowslots++;

      if(ch==0){
          *output32++ = host_safe_hw_ptr(&block);
          // shortcut
          // there's likely to be a lot of spaces on the screen.
          // if this character is a space, just use this predefined zero block rather than the calculation below
      }
      else{
        *output32++ = host_safe_hw_ptr(dbase + ch * FONT_HEIGHT * FONT_WIDTH_WORDS);
      }

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

    do{

        handle_new_character(read_key_from_buffer());
        // or for analysing what comes in
        //print_ascii_value(read_key_from_buffer());

    }while(key_ready());

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


// Wire a LED on GPIO 5 (01x10 extra connector) to help debugging
// the source code by calling set_debug_set() which blink the LED count's time
#define LED_DEBUG 5
void set_debug_led( int count ){
  gpio_init( LED_DEBUG );
  gpio_set_dir(LED_DEBUG, GPIO_OUT);
  for( int i=0; i<10; i++){
      gpio_put(LED_DEBUG,true);
      sleep_ms( 20 );
      gpio_put(LED_DEBUG,false);
      sleep_ms( 20 );
  }
  sleep_ms( 500 );
  for( int i=0; i<count; i++){
    gpio_put(LED_DEBUG,true);
    sleep_ms( 300 );
    gpio_put(LED_DEBUG,false);
    sleep_ms( 300 );
  }
}


int main(void) {
    //gpio_put(27, 0);

    LED_status = 3;  // blinking

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
            read_data_from_flash();
            break;

        case 1:
            colour_preference = GREEN1;     // A
            write_data_to_flash();
            break;
        case 2:
            colour_preference = DARKAMBER;  // B
            write_data_to_flash();
            break;
        case 4:
                                            // C
            break;
        case 3:
            colour_preference = GREEN2;     // A+B
            write_data_to_flash();
            break;
        case 5:
            colour_preference = GREEN3;     // C+A
            write_data_to_flash();
            break;
        case 6:
            colour_preference = LIGHTAMBER; // C+B
            write_data_to_flash();
            break;
        case 7:
                                             // A=B=C
            break;

        default:
            break;
        }

    // AFTER   reading and writing
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID,false,false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);


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


    keybuffer1.length=2000;
    keybuffer1.take=0;
    keybuffer1.insert=0;

    prepare_text_buffer();
    display_terminal(); // display terminal entry screen
    video_main();
    tusb_init(); // initialize tinyusb stack

    char _ch = 0;
    bool old_menu = false; // used to trigger when is_menu is changed

    while(true){
        // do character stuff here on core 0

        // for serial over USB  (don't forget to make pico_enable_stdio_usb(picoterm 1))
        // usb_serial_task();

        // TinyUsb Host Task
        //   (see process_kdb_report() callback and pico_key_down() here below)
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
    }
    return 0;
}


//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;


  switch(LED_status){
    case 0:
     board_led_write(false);
     break;
    case 1:
     board_led_write(true);
     break;
    case 3:

      // Blink every interval ms
      if ( board_millis() - start_ms > interval_ms) {
        start_ms += interval_ms;

        board_led_write(led_state);
        led_state = 1 - led_state; // toggle
      }
    break;
  }


}

//--------------------------------------------------------------------+
// USB Keyboard clue
//--------------------------------------------------------------------+

static bool capslock_key_down_in_last_report = false;
static bool capslock_key_down_in_this_report = false;
static bool capslock_on = false;

static uint8_t const keycode2ascii[128][3] =  { PM_KEYCODE_TO_ASCII };


static bool scancode_is_mod(int scancode) {
    static const uint8_t mods[] = {
            SDL_SCANCODE_LCTRL,
            SDL_SCANCODE_RCTRL,
            SDL_SCANCODE_LALT,
            SDL_SCANCODE_RALT,
            SDL_SCANCODE_LSHIFT,
            SDL_SCANCODE_RSHIFT,
    };
    for(int i=0;i<count_of(mods); i+= 2) {
        if(scancode== mods[i]){
          return true;
        }
    }
    // default
    return false;
}

//--------------------------------------------------------------------+
// This is the 'glue'
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


//--------------------------------------------------------------------+
// USB stuff
//--------------------------------------------------------------------+


#define MAX_REPORT  4
#define debug_printf(fmt,...) ((void)0)

// Each HID instance can has multiple reports
static struct
{
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
//static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    debug_printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

    LED_status = 0;  // off

    // Interface protocol (hid_interface_protocol_enum_t)
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    debug_printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);
//    printf("%d USB: device %d connected, protocol %s\n", time_us_32() - t0 , dev_addr, protocol_str[itf_protocol]);

    // By default host stack will use activate boot protocol on supported interface.
    // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
    if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
    {
        hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
        debug_printf("HID has %u reports \r\n", hid_info[instance].report_count);
    }

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        debug_printf("Error: cannot request to receive report\r\n");
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    debug_printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    printf("USB: device %d disconnected\n", dev_addr);

    LED_status = 3;  // blinking
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol)
    {
        case HID_ITF_PROTOCOL_KEYBOARD:
            TU_LOG2("HID receive boot keyboard report\r\n");
            process_kbd_report( (hid_keyboard_report_t const*) report );
            break;


        default:
            // Generic report requires matching ReportID and contents with previous parsed report info
            process_generic_report(dev_addr, instance, report, len);
            break;
    }

    // continue to request to receive report
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        debug_printf("Error: cannot request to receive report\r\n");
    }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
    for(uint8_t i=0; i<6; i++)
    {
        if (report->keycode[i] == keycode)  return true;
    }

    return false;
}



static void process_kbd_report(hid_keyboard_report_t const *report)
{
    static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for(uint8_t i=0; i<6; i++)
    {
        if ( report->keycode[i] )
        {


            capslock_key_down_in_this_report = false;
            for(uint8_t i=0; i<6; i++){
                if ( find_key_in_report(report, HID_KEY_CAPS_LOCK)){
                capslock_key_down_in_this_report = true;
                }
            }
            if(capslock_key_down_in_this_report==true && capslock_key_down_in_last_report==false){
                // toggle the value
                capslock_on = !capslock_on;
            }


            if ( find_key_in_report(&prev_report, report->keycode[i]) )
            {
                // exist in previous report means the current key is holding
            }else
            {
                // not existed in previous report means the current key is pressed
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                bool const is_altgr = report->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
                // capslock_on

                int modifier = 0;
                modifier = modifier | (is_shift ? WITH_SHIFT : 0);
                modifier = modifier | (is_altgr ? WITH_ALTGR : 0);
                modifier = modifier | (is_ctrl ? WITH_CTRL : 0);
                modifier = modifier | (capslock_on ? WITH_CAPSLOCK : 0);

                pico_key_down(report->keycode[i], 0, modifier);
            }
        }
        // Check for key depresses (i.e. was present in prev report but not here)
        if (prev_report.keycode[i]) {
            // If not present in the current report then depressed
            if (!find_key_in_report(report, prev_report.keycode[i]))
            {
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                bool const is_altgr = report->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
                // capslock_on

                int modifier = 0;
                modifier = modifier | (is_shift ? WITH_SHIFT : 0);
                modifier = modifier | (is_altgr ? WITH_ALTGR : 0);
                modifier = modifier | (is_ctrl ? WITH_CTRL : 0);
                modifier = modifier | (capslock_on ? WITH_CAPSLOCK : 0);

                pico_key_up(prev_report.keycode[i], 0, modifier);
            }
        }
    }

    /*
    // synthesize events for modifier keys
    static const uint8_t mods[] = {
            KEYBOARD_MODIFIER_LEFTCTRL, SDL_SCANCODE_LCTRL,
            KEYBOARD_MODIFIER_RIGHTCTRL, SDL_SCANCODE_RCTRL,
            KEYBOARD_MODIFIER_LEFTALT, SDL_SCANCODE_LALT,
            KEYBOARD_MODIFIER_RIGHTALT, SDL_SCANCODE_RALT,
            KEYBOARD_MODIFIER_LEFTSHIFT, SDL_SCANCODE_LSHIFT,
            KEYBOARD_MODIFIER_RIGHTSHIFT, SDL_SCANCODE_RSHIFT,
    };
    for(int i=0;i<count_of(mods); i+= 2) {
        check_mod(report->modifier, prev_report.modifier, mods[i], mods[i+1]);
    }
    */

    prev_report = *report;
}





//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void) dev_addr;

    uint8_t const rpt_count = hid_info[instance].report_count;
    tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
    tuh_hid_report_info_t* rpt_info = NULL;

    if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
    {
        // Simple report without report ID as 1st byte
        rpt_info = &rpt_info_arr[0];
    }else
    {
        // Composite report, 1st byte is report ID, data starts from 2nd byte
        uint8_t const rpt_id = report[0];

        // Find report id in the arrray
        for(uint8_t i=0; i<rpt_count; i++)
        {
            if (rpt_id == rpt_info_arr[i].report_id )
            {
                rpt_info = &rpt_info_arr[i];
                break;
            }
        }

        report++;
        len--;
    }

    if (!rpt_info)
    {
        debug_printf("Couldn't find the report info for this report !\r\n");
        return;
    }

    // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
    // - Keyboard                     : Desktop, Keyboard
    // - Mouse                        : Desktop, Mouse
    // - Gamepad                      : Desktop, Gamepad
    // - Consumer Control (Media Key) : Consumer, Consumer Control
    // - System Control (Power key)   : Desktop, System Control
    // - Generic (vendor)             : 0xFFxx, xx
    if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
    {
        switch (rpt_info->usage)
        {
            case HID_USAGE_DESKTOP_KEYBOARD:
                TU_LOG1("HID receive keyboard report\r\n");
                // Assume keyboard follow boot report layout
                process_kbd_report( (hid_keyboard_report_t const*) report );
                break;



            default: break;
        }
    }
}
