/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 &21
 * Shiela Dixon, https://peacockmedia.software
 *
 * main.c handles the ins and outs
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


#include <string.h>
#include <stdbool.h>
#include <stdint.h>


#ifndef _PICOTERM_CORE_H
#define _PICOTERM_CORE_H


static uint8_t custom_bitmap[768] = {
        0b11111000,
        0b10001100,
        0b10001100,
        0b10001100,
        0b11111100,
        0b01111100   };

// pixel data
#define ARBITRARY 0
#define BITMAPDATA 1
#define UDCHAR 2

static int data_purpose;
static int current_udchar;
static uint32_t data_bytes_expected;


void terminal_init();
void terminal_reset();

void reset_escape_sequence();
void clear_escape_parameters();
void esc_sequence_received();
void handle_new_character(unsigned char asc);

char get_bell_state();
void set_bell_state(char state);

void print_logo_element(int x,int scanlineNumber);
void print_ascii_value(unsigned char asc);

#endif
