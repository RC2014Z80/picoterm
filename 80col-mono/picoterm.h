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
#include <stdlib.h>
#include <stdbool.h>



#ifndef _PICOTERM_H
#define _PICOTERM_H

#define BEL         0x07 //BEL	7	007	0x07	\a	^G	Terminal bell
#define BSP         0x08 //BS	8	010	0x08	\b	^H	Backspace
#define HT          0x09 //HT	9	011	0x09	\t	^I	Horizontal TAB
#define SPC         0x20

#define LF          0x0a //LF	10	012	0x0A	\n	^J	Linefeed (newline)
#define VT          0x0b //VT	11	013	0x0B	\v	^K	Vertical TAB
#define FF          0x0c //FF	12	014	0x0C	\f	^L	Formfeed (also: New page NP)
#define CR          0x0d //CR	13	015	0x0D	\r	^M	Carriage return

#define ESC         0x1b //ESC	27	033	0x1B	<none>	^[	Escape character
#define DEL         0x7f //DEL	127	177	0x7F	<none>	<none>	Delete character

unsigned char slop_character(int x,int y);
unsigned char * slotsForRow(int y);
unsigned char * slotsForInvRow(int y);
unsigned char * slotsForBlkRow(int y);

void reset_terminal();
void prepare_text_buffer();
void display_terminal();
void display_config();
void display_help();
void display_charset();

// bool get_csr_blink_state(); moved to picoterm_cursor
// void set_csr_blink_state(bool state);
void refresh_cursor();
void clear_cursor();
void print_cursor();
void handle_new_character(unsigned char ch);
void print_string(char str[]);
void __print_string(char str[], bool strip_nupetscii );
// for menu support
char read_key();
char handle_default_input();
char handle_config_input(); // specialzed for MENU_CONFIG
bool key_ready(); // in main.c
unsigned char read_key_from_buffer(); // in main.c
// for debugging purposes
void print_ascii_value(unsigned char asc);

#endif
