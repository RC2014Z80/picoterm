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
//#include "font.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "../common/pmhid.h"



#ifndef _MAIN_H
#define _MAIN_H

#define LED             25
#define UART_ID         uart1   // also see hid_app.c
#define UART_TX_PIN     20
#define UART_RX_PIN     21

#define MENU_CONFIG    0x01 // support several menu
#define MENU_CHARSET   0x02 // display current charset
#define MENU_HELP      0x03 // display the HELP menu
#define MENU_COMMAND   0x04 // display Command interpreter

#define USB_POWER_GPIO 26 // this GPIO can be used with a MOSFET to power-up USB
#define USB_POWER_DELAY 5000 // ms

#define BUZZER_GPIO 27 // active buzzer

uint32_t start_time;

static void pico_key_down(int scancode, int keysym, int modifiers);
static void pico_key_up(int scancode, int keysym, int modifiers);

// void build_font(); not defined in the 40 col version
void render_on_core1();
void stop_core1();

#endif // _MAIN_H
