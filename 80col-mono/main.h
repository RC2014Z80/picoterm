/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO 20 & 21
 * Shiela Dixon, https://peacockmedia.software
 *
 * much of what's in this main file is taken from the VGA textmode example
 * and the TinyUSB hid_app
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


#include <stdio.h>

#include <stdlib.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "font.h" // Looks in under comment for 40 columns version
#include "hardware/irq.h"
#include <stdint.h>


//#include "bsp/board.h"
//#include "tusb.h"

#include "../common/pmhid.h"

#ifndef _MAIN_H
#define _MAIN_H


#define MENU_CONFIG    0x01 // support several menu
#define MENU_CHARSET   0x02 // display current charset
#define MENU_HELP      0x03 // display the HELP menu
#define MENU_COMMAND   0x04 // Key-in interpreter command


static uint32_t start_time;

static void pico_key_down(int scancode, int keysym, int modifiers);
static void pico_key_up(int scancode, int keysym, int modifiers);

void select_graphic_font( uint8_t font_id );
void build_font( uint8_t font_id );
// void read_data_from_flash();
// void write_data_to_flash();
void render_on_core1();
void stop_core1();

#endif // _MAIN_H
