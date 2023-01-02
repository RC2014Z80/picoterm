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

#ifndef _PICOTERM_CORE_H
#define _PICOTERM_CORE_H


unsigned char slop_character(int x,int y);
unsigned char * slotsForRow(int y);
unsigned char * slotsForInvRow(int y);
unsigned char * slotsForBlkRow(int y);

void reset_terminal();
void prepare_text_buffer();

char get_bell_state();
void set_bell_state(char state);
// MOVED! bool get_csr_blink_state(); moved to picoterm_cursor
// MOVED! void set_csr_blink_state(bool state);
void refresh_cursor();
void clear_cursor();
void print_cursor();
void handle_new_character(unsigned char ch);
// Moved! void print_string(char str[]);
// Moved! void __print_string(char str[], bool strip_nupetscii );
// Moved! for menu support
// Moved! char read_key();
// moved! char handle_default_input();
// moved! char handle_config_input(); // specialzed for MENU_CONFIG
// ?????  bool key_ready(); // in keybd.c
unsigned char read_key_from_buffer(); // in main.c

// for debugging purposes
void print_ascii_value(unsigned char asc);

#endif