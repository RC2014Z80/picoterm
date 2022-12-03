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

#define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BSP         0x08
#define LF          0x0a
#define CR          0x0d
#define FF          0x0c

unsigned char slop_character(int x,int y);
unsigned char * slotsForRow(int y);
void prepare_text_buffer();
void display_terminal();
void display_config();
void display_help();
void display_nupetscii();
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
